#include "stdafx.hpp"
#include "GeneticAlgo.hpp"

namespace GridGALib
{
	GeneticAlgo::GeneticAlgo(std::string filesLocation, zmq::context_t& zmqContext)
    :
        mGenomeCache(boost::make_shared<std::deque<GenomePtr> >()),
        mFilesLocation(filesLocation),
        mGAPort(55577),
        mZmqContext(zmqContext),
        mTimeoutMinutes(1), 
        mPrintBestNum(20),
        mUsingRecordedSignals(false),
        mCross(static_cast<CrossFunc>(0)),
        mGetGenomeConfig(static_cast<GetGenomeConfigFunc>(0))
    {
       // mHTCondor.reset(new HTCondor(filesLocation, zmqContext));
        srand(static_cast<boost::uint32_t>(time(NULL)));
    }

    //______________________________________________________________________________________________________________

    bool GeneticAlgo::ReadConfig(void)
    {
        boost::replace_all(mFilesLocation, "\\", "/");

        std::string configTemplateFileName = CommonLib::GetConfigFileNameIfExists(mFilesLocation);
        if (configTemplateFileName == "")
        {
            FILE_LOG(logERROR) << __FUNCTION_NAME__ << "Cannot find a config in " << mFilesLocation << "!";
            return false;
        }

        mCacheFile = mFilesLocation + "/genetic-algo-cache.xml";

        boost::property_tree::ptree pt;
        boost::property_tree::read_xml(configTemplateFileName, pt);

        mGAPort = CommonLib::GetOptionalParameter<boost::int32_t>("config.genetic-algo.ga-server-port", pt, 55566);

        std::string objectiveFunction = CommonLib::GetOptionalParameter<std::string>("config.genetic-algo.objective-function", pt, "sharpe");
        
        /*if (boost::iequals(objectiveFunction, "sharpe"))
        {
            mObjectiveFunction = OBJECTIVE_SHARPE;
        }
        else if (boost::iequals(objectiveFunction, "pnl"))
        {
            mObjectiveFunction = OBJECTIVE_PNL;
        }
        else if (boost::iequals(objectiveFunction, "accuracy"))
        {
            mObjectiveFunction = OBJECTIVE_ACCURACY;
        }
        else
        {
            FILE_LOG(logERROR) << __FUNCTION_NAME__ << "Unknown objective: " << objectiveFunction << ". Using default of sharpe.";
            mObjectiveFunction = OBJECTIVE_SHARPE;
        }*/

        mTimeoutMinutes = CommonLib::GetOptionalParameter<std::size_t>("config.genetic-algo.timeout-minutes", pt, 120);
        mPopulationSize = CommonLib::GetOptionalParameter<std::size_t>("config.genetic-algo.population-size", pt, 20);
        mMutationProbability = CommonLib::GetOptionalParameter<std::size_t>("config.genetic-algo.mutation-probability", pt, 20);
        mNumBreedersPercent = CommonLib::GetOptionalParameter<double>("config.genetic-algo.num-breeders-percent", pt, 50.0);
        if (CommonLib::DoublesEqual(mNumBreedersPercent, 0.0))
        {
            mNumBreedersPercent = 0.5;
        }
        else
        {
            mNumBreedersPercent = mNumBreedersPercent / 100.0;
        }
        mMinNumBreeders = CommonLib::GetOptionalParameter<std::size_t>("config.genetic-algo.min-num-breeders", pt, 50);
        mNumNewRandomGenomes = CommonLib::GetOptionalParameter<std::size_t>("config.genetic-algo.num-new-random-genomes", pt, 2);
        mNumGenerations = CommonLib::GetOptionalParameter<std::size_t>("config.genetic-algo.num-generations", pt, 5);

        mUsingRecordedSignals = CommonLib::GetOptionalBoolParameter("config.backtest.use-recorded-signals", pt, false);

        //mExecutable = CommonLib::GetOptionalParameter<std::string>("config.genetic-algorithm.executable", pt, "DeepThought");

        //mConfigXMLTemplateFileName = CommonLib::GetOptionalParameter<std::string>("config.genetic-algo.xml-template", pt, "not-set");

        std::string executionType = CommonLib::GetOptionalParameter<std::string>("config.genetic-algo.execution-type", pt, "not-set");
        if (boost::iequals(executionType, "htcondor"))
        {
            mHTCondor.reset(new HTCondor(mFilesLocation, mZmqContext));
            if (!mHTCondor->ReadConfig(pt))
            {
                return false;
            }
        }
        else
        {
            FILE_LOG(logERROR) << __FUNCTION_NAME__ << "Execution type not set! Please check the entry config.genetic-algo.execution-type in the config.";
            return false;
        }

        // read the parameters
        mParameterMap = boost::make_shared<GAParameterMap>();

        boost::property_tree::ptree& configPt = pt.get_child("config.genetic-algo");

        for (boost::property_tree::ptree::const_iterator itr=configPt.begin(); itr!=configPt.end(); ++itr)
        {
            if (boost::iequals(itr->first, "parameter"))
            {
                if (boost::iequals(itr->second.get_child("<xmlattr>").get<std::string>("type"), "integer"))
                {
                    (*mParameterMap)[itr->second.get_child("<xmlattr>").get<std::string>("id")] = 
                        boost::make_shared<GenomeParameterContinuous>(
                        itr->second.get_child("<xmlattr>").get<std::string>("id"),
                        itr->second.get_child("<xmlattr>").get<boost::int32_t>("low"),
                        itr->second.get_child("<xmlattr>").get<boost::int32_t>("high"),
                        itr->second.get_child("<xmlattr>").get<boost::int32_t>("step"));
                    FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "Loaded Integer genome parameter " << 
                        (*mParameterMap)[itr->second.get_child("<xmlattr>").get<std::string>("id")]->GetIdentifier();
                }
                else if (boost::iequals(itr->second.get_child("<xmlattr>").get<std::string>("type"), "exp-2"))
                {
                    (*mParameterMap)[itr->second.get_child("<xmlattr>").get<std::string>("id")] = 
                        boost::make_shared<GenomeParameterExp2>(
                        itr->second.get_child("<xmlattr>").get<std::string>("id"),
                        itr->second.get_child("<xmlattr>").get<boost::int32_t>("low"),
                        itr->second.get_child("<xmlattr>").get<boost::int32_t>("high"),
                        itr->second.get_child("<xmlattr>").get<boost::int32_t>("step"));
                    FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "Loaded Exp-2 genome parameter " << 
                        (*mParameterMap)[itr->second.get_child("<xmlattr>").get<std::string>("id")]->GetIdentifier();
                }
                else if (boost::iequals(itr->second.get_child("<xmlattr>").get<std::string>("type"), "categorical"))
                {
                    (*mParameterMap)[itr->second.get_child("<xmlattr>").get<std::string>("id")] = 
                        boost::make_shared<GenomeParameterCategorical>(
                        itr->second.get_child("<xmlattr>").get<std::string>("id"),
                        itr->second.get_child("<xmlattr>").get<std::string>("values"));
                    FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "Loaded Categorical genome parameter " << 
                        (*mParameterMap)[itr->second.get_child("<xmlattr>").get<std::string>("id")]->GetIdentifier();
                }
                else
                {
                    FILE_LOG(logERROR) << __FUNCTION_NAME__ << "Unknown GA parameter type: " << itr->second.get_child("<xmlattr>").get<std::string>("type");
                }
            }
        }

        if (!mCross)
        {
            FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "Using default genome cross function: CrossBySlicing";
            mCross = boost::bind(&GeneticAlgo::CrossBySlicing, this, _1, _2, _3, _4);
        }

        FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "Loaded config - " << mParameterMap->size() << " parameters to optimise";
        return true;
    }

    //______________________________________________________________________________________________________________

    GAParameterMapPtr GeneticAlgo::GetRandomGAParameters()
    {
        GAParameterMapPtr parameterMap = boost::make_shared<GAParameterMap>();

        BOOST_FOREACH(GAParameterMap::value_type& parameter, *mParameterMap)
        {
            if (parameter.second->GetParameterType() == PARAMETER_TYPE_INTEGER)
            {
                (*parameterMap)[parameter.second->GetIdentifier()] = GetGenomeParameterCopy<GenomeParameterContinuous>((parameter.second));
            }
            else if (parameter.second->GetParameterType() == PARAMETER_TYPE_EXP_2)
            {
                (*parameterMap)[parameter.second->GetIdentifier()] = GetGenomeParameterCopy<GenomeParameterExp2>((parameter.second));
            }
            else if (parameter.second->GetParameterType() == PARAMETER_TYPE_CATEGORICAL)
            {
                (*parameterMap)[parameter.second->GetIdentifier()] = GetGenomeParameterCopy<GenomeParameterCategorical>((parameter.second));
            }
        }

        BOOST_FOREACH(GAParameterMap::value_type& parameter, *parameterMap)
        {
            parameter.second->SetRandomValue();
        }

        return parameterMap;
    }

    //______________________________________________________________________________________________________________

    bool GeneticAlgo::GAParametersOk(const GAParameterMapPtr parameters)
    {
        // TODO
        // Check that parameters make sense. Not all combinations of params are valid.
        return true;
    }

    //______________________________________________________________________________________________________________

    void GeneticAlgo::Evolve(void)
    {
        mGenerationNumber = 1;

        RestoreState();      

        while (mGenerationNumber <= mNumGenerations)
        {
            Genome::SetGenerationNumber(static_cast<boost::int32_t>(mGenerationNumber));

            FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "Generation " << mGenerationNumber;

            GenomeList genomesToTest = NextGeneration();

            if (genomesToTest->size() == 0)
            {
                FILE_LOG(logWARNING) << __FUNCTION_NAME__ << "No genomes to test. Exiting.";
                break;
            }

            mHTCondor->ExecuteGeneration(genomesToTest, mGenomeCache, mGenerationNumber);
            StoreState();
            ++mGenerationNumber;

        }
    }

    //______________________________________________________________________________________________________________

    GeneticAlgo::~GeneticAlgo(void)
    {
    }

    //______________________________________________________________________________________________________________

    GenomePtr GeneticAlgo::CreateRandomGenome()
    {
        GenomePtr genome(boost::make_shared<Genome>());
        genome->SetIntParameterList(GetRandomGAParameters());
        return genome;
    }

    //______________________________________________________________________________________________________________

    void GeneticAlgo::CrossBySlicing(const GenomePtr parent1, const GenomePtr parent2, 
        GenomePtr child1, GenomePtr child2)
    {
        // pick a random place to cross the two parents
        boost::int32_t crossPoint = rand() % (parent1->GetParameters()->size() - 1);
        FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "- Crossing at index " << crossPoint;
        boost::int32_t index = 0;
        BOOST_FOREACH(const GAParameterMap::value_type& parameter, *(parent1->GetParameters()))
        {
            if (index <= crossPoint)
            {
                child1->SetInternalParameterValue(parameter.first, parent1->GetInternalParameterValue(parameter.first));
                child2->SetInternalParameterValue(parameter.first, parent2->GetInternalParameterValue(parameter.first));
            }
            else
            {
                child1->SetInternalParameterValue(parameter.first, parent2->GetInternalParameterValue(parameter.first));
                child2->SetInternalParameterValue(parameter.first, parent1->GetInternalParameterValue(parameter.first));
            }
            index++;
        }
    }

    //______________________________________________________________________________________________________________

    void GeneticAlgo::CrossBySwap(const GenomePtr parent1, const GenomePtr parent2, 
        GenomePtr child1, GenomePtr child2)
    {
        // pick a random place to switch a single value between the two parents
        boost::int32_t swapIndex = rand() % (parent1->GetParameters()->size() - 1);
        FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "- Swapping at index " << swapIndex;
        boost::int32_t index = 0;
        BOOST_FOREACH(const GAParameterMap::value_type& parameter, *(parent1->GetParameters()))
        {
            if (index == swapIndex)
            {
                child1->SetInternalParameterValue(parameter.first, parent1->GetInternalParameterValue(parameter.first));
                child2->SetInternalParameterValue(parameter.first, parent2->GetInternalParameterValue(parameter.first));
            }
            else
            {
                child1->SetInternalParameterValue(parameter.first, parent2->GetInternalParameterValue(parameter.first));
                child2->SetInternalParameterValue(parameter.first, parent1->GetInternalParameterValue(parameter.first));
            }
            index++;
        }
    }

    //______________________________________________________________________________________________________________

    bool GeneticAlgo::AddGenomeToPopulation(GenomeList genomesToTest, GenomePtr newGenome)
    {
        if (!GAParametersOk(newGenome->GetParameters()))
        {
            return false;
        }

        // will return false if an individual with the same genome already exists and is complete
        // note that we only call this routine after a generation has been run, so there are no 'in-progress' genonmes
        BOOST_FOREACH(GenomePtr genome, *mGenomeCache)
        {
            bool match = true;
            BOOST_FOREACH(const GAParameterMap::value_type& parameter, *(newGenome->GetParameters()))
            {
                if (parameter.second->GetHasValue() && parameter.second->InternalValue() != genome->GetInternalParameterValue(parameter.first))
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                return false;
            }
        }

        genomesToTest->push_back(newGenome);
        return true;
    }

    //______________________________________________________________________________________________________________

    void GeneticAlgo::RemoveIncomleteGenomes(void)
    {
        for (std::deque<GenomePtr>::iterator genomeItr = mGenomeCache->begin() ; genomeItr != mGenomeCache->end() ; )
        {
            if (!(*genomeItr)->IsComplete())
            {
                genomeItr = mGenomeCache->erase(genomeItr);
            }
            else
            {
                ++genomeItr;
            }
        }
    }

    //______________________________________________________________________________________________________________

    GenomeList GeneticAlgo::NextGeneration(void)
    {
        std::size_t addedCount = 0;
        std::size_t rejectionCount = 0;

        // log genomes that we didn't receive results from
        std::size_t numIncomplete = 0;
        BOOST_FOREACH(GenomePtr genome, *mGenomeCache)
        {
            if (!genome->IsComplete())
            {
                FILE_LOG(logWARNING) << "Genome[" << genome->GetGenomeID() << "] returned no result. " << genome->ToString();
                ++numIncomplete;
            }
        }

        RemoveIncomleteGenomes();

        GenomeList genomesToTest = boost::make_shared<std::deque<GenomePtr> >();

        // initialise the initial population with random genomes
        if (mGenomeCache->size() == 0)
        {
            std::size_t initialRejectionCount = 0;
            // initialise population
            while (genomesToTest->size() < mPopulationSize)
            {
                if (AddGenomeToPopulation(genomesToTest, CreateRandomGenome()))
                {
                    addedCount++;
                }
                else
                {
                    ++initialRejectionCount;
                    if (initialRejectionCount == 10000)
                    {
                        break;
                    }
                }
            }
            return genomesToTest;
        }

        // rejectionCount is used to prevent us getting into an infinite loop if we are unable to add any more genomes.
        
        rejectionCount = 0;

        // create some new random genomes
        std::size_t numAdded = 0;
        std::size_t i = 0;
        while (i < mNumNewRandomGenomes)
        {
            if (!AddGenomeToPopulation(genomesToTest, CreateRandomGenome()))
            {
                rejectionCount++;
                if (rejectionCount > 1000)
                {
                    break;
                }
            }
            else
            {
                addedCount++;
                numAdded++;
                ++i;
            }
        }

        FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "- Added " << numAdded << " new random genomes";

        rejectionCount = 0;
        std::size_t numMutations = 0;

        std::size_t numBreeders = std::max(static_cast<size_t>(mNumBreedersPercent * mGenomeCache->size()), mMinNumBreeders);
       
        numBreeders  = std::min(numBreeders, mGenomeCache->size()-1);

        // breed and mutate
        while ((genomesToTest->size() < mPopulationSize) && (rejectionCount < 1000))
        {
            GenomePtr parent1 = mGenomeCache->at(rand() % numBreeders);
            GenomePtr parent2 = mGenomeCache->at(rand() % numBreeders);
            if (parent1->IsComplete() && parent2->IsComplete())
            {
                if (parent1.get() != parent2.get())
                {
                    GenomePtr child1 = CreateRandomGenome();
                    GenomePtr child2 = CreateRandomGenome();
                    mCross(parent1, parent2, child1, child2);

                    //FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "Cross:\n" << 
                    //    "Parent 1 : " << parent1->ToString() << "\n" <<
                    //    "Parent 2 : " << parent2->ToString() << "\n" <<
                    //    "Child 1  : " << child1->ToString() << "\n" <<
                    //    "Child 2  : " << child2->ToString();

                    if (child1->Mutate(mMutationProbability))
                    {
                        ++numMutations;
                    }

                    if (child2->Mutate(mMutationProbability))
                    {
                        ++numMutations;
                    }

                    if (!AddGenomeToPopulation(genomesToTest, child1))
                    {
                        rejectionCount++;
                    }
                    else
                    {
                        ++addedCount;
                    }

                    if (genomesToTest->size() < mPopulationSize)
                    {
                        if (!AddGenomeToPopulation(genomesToTest, child2))
                        {
                            ++rejectionCount;
                        }
                        else
                        {
                            ++addedCount;
                        }
                    }
                }
                else
                {
                    ++rejectionCount;
                }
            }
            else
            {
                ++rejectionCount;
            }
        }

        FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "Number of mutants: " << numMutations;

        if (rejectionCount >= 1000)
        {
            FILE_LOG(logWARNING) << __FUNCTION_NAME__ << "- Rejection count is " << rejectionCount;
        }

        if (genomesToTest->size() < mPopulationSize)
        {
            FILE_LOG(logWARNING) << __FUNCTION_NAME__ << "- Population size is " << genomesToTest->size();
        }

        return genomesToTest;
    }

    //______________________________________________________________________________________________________________

//    std::string GeneticAlgo::WriteSubmitFile(const std::deque<GenomePtr>& genomePopulation) 
//    {
//        std::ostringstream s;
//        s << mFilesLocation << "/generation-" << mGenerationNumber << ".submit";
//        std::ofstream submitFile(s.str().c_str());
//
//        submitFile << "Executable = " << mExecutable << "\n";
//        submitFile << "Universe = vanilla\n";
//        submitFile << "should_transfer_files = yes\n";
//        submitFile << "stream_error = false\n";
//        submitFile << "stream_input = false\n";
//        submitFile << "stream_output = false\n";
//        submitFile << "should_transfer_files = YES\n";
//        submitFile << "when_to_transfer_output = ON_EXIT_OR_EVICT\n";    
//
//        std::ostringstream logFileName;
//        logFileName << "genetic-algo.condor." << mGenerationNumber << ".log";
//        try
//        {
//            if (boost::filesystem::exists(logFileName.str()))
//            {       
//                boost::filesystem::remove(logFileName.str());
//            }
//        }
//        catch (std::exception& e)
//        {
//            FILE_LOG(logERROR) << __FUNCTION_NAME__ << "" << e.what();
//            std::cerr << __FUNCTION_NAME__ << "" << e.what() << std::endl;
//            exit(-1);
//        }
//
//        submitFile << "log = " << logFileName.str() << "\n";
//
//        std::ostringstream generationSubDir;
//        generationSubDir << mFilesLocation << "/generation-" << mGenerationNumber;
//
//        if (boost::filesystem::exists(generationSubDir.str()))
//        {
//            FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "- Emptying directory " << generationSubDir.str() << ".";
//            try
//            {
//                boost::filesystem::remove_all(generationSubDir.str());
//                // Windows returns an access denied if we immediately attempt to recreate the directory hence the
//                // reason for this silly code.
//                bool fail = true;
//                while (fail)
//                {
//                    try
//                    {
//                        fail = false;
//                        boost::filesystem::create_directory(generationSubDir.str());
//                    }
//                    catch (std::exception& e)
//                    {
//                        FILE_LOG(logERROR) << __FUNCTION_NAME__ << "" << e.what();
//                        fail =true;
//                    }
//                }
//            }
//            catch (std::exception& e)
//            {
//                FILE_LOG(logERROR) << __FUNCTION_NAME__ << "- " << e.what();
//                exit(-1);
//            }
//            FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "- " << generationSubDir.str() << " has been deleted.";
//        }
//
//        if (!boost::filesystem::exists(generationSubDir.str()))
//        {
//            FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "- Creating directory " << generationSubDir.str();
//            boost::filesystem::create_directory(generationSubDir.str());
//            FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "- " << generationSubDir.str() << " has been created.";
//        }
//
//        std::string paramPrefix("--");
//        std::string valuePrefix(" ");
//
//        BOOST_FOREACH(GenomePtr genome, genomePopulation)
//        {
//            if (!genome->IsComplete())
//            {
//                
//                std::ostringstream outFileName(genome->GetGenomeID());
//                //std::ostringstream fullConfigFileName;
//
//                //fullConfigFileName << generationSubDir.str() << "/" << configFileName << "xml";
//                //std::ofstream outFile(fullConfigFileName.str().c_str());
//
//                std::ostringstream transferInputFiles;
//
//                if (!mGetGenomeConfig)
//                {
//                    transferInputFiles <<  mGetGenomeConfig(//CommonLib::GetConfigFileNameIfExists(mFilesLocation), 
//                        genome, //genome->GetParameters(), 
//                        //static_cast<boost::int32_t>(genome->GetGenomeID()), 
//                        generationSubDir.str());
//                }
//
//                //outFile << configXML << std::endl;
//                //outFile.close();
//                submitFile << "Arguments = " << genome->GetCommandLineArguments(paramPrefix, valuePrefix) << "\n";
//                submitFile << "Output = " << generationSubDir.str() << "/" << outFileName.str() << "out\n";
//                submitFile << "Error = " << generationSubDir.str() << "/" << outFileName.str() << "err\n";
//                
//                submitFile << "transfer_input_files = " << transferInputFiles.str() << GetPythonFiles();
//     
//                submitFile << "\n";
//#ifdef _WIN32
//                submitFile << "Requirements   = (OpSys == \"WINDOWS\" && Arch ==\"X86_64\") || (OpSys == \"WINDOWS\" && Arch ==\"INTEL\")\n";
//#endif
//                submitFile << "transfer_output_files = results.zip\n";
//                // copy into the 'generation' directory
//                submitFile << "transfer_output_remaps = \"results.zip = " << generationSubDir.str() << "/" << genome->GetGenomeID() << ".results.zip\"\n";
//                submitFile << "Queue\n";
//            }
//        }
//
//        submitFile.close();
//        return s.str();
//    }

    //______________________________________________________________________________________________________________

    //std::string GeneticAlgo::GetPythonFiles(void)
    //{
    //    boost::filesystem::directory_iterator end_itr; // Default ctor yields past-the-end
    //    std::ostringstream s;
    //    for( boost::filesystem::directory_iterator i( mFilesLocation ); i != end_itr; ++i )
    //    {
    //        // Skip if not a file
    //        if( !boost::filesystem::is_regular_file( i->status() ) ) continue;

    //        // Skip if not a python file
    //        if (!boost::iequals(i->path().extension().string(), ".py")) continue;

    //        std::string fileName = i->path().string();
    //        s << "," << fileName;
    //    }
    //    return s.str();
    //}

    ////______________________________________________________________________________________________________________

    //void GeneticAlgo::SubmitToCluster(const std::string& submitFileName)
    //{
    //    std::cerr << __FUNCTION_NAME__ << std::endl;
    //    std::ostringstream cmd;
    //    cmd << "condor_submit " << submitFileName;
    //    FILE_LOG(logINFO) << "Executing " << cmd.str();
    //    std::system(cmd.str().c_str());
    //    
    //    // parse the log and get the cluster id
    //    std::ostringstream condorLogFile;
    //    condorLogFile << "genetic-algo.condor." << mGenerationNumber << ".log";
    //    std::ifstream condorLog(condorLogFile.str().c_str());
    //    std::string line;
    //    while ( condorLog.good() )
    //    {
    //        getline(condorLog,line);
    //        boost::trim(line);
    //        if (!line.empty())
    //        {
    //            std::vector<std::string> stringTokens;
    //            boost::split(stringTokens, line, boost::is_any_of(" "));
    //            BOOST_FOREACH(std::string& token, stringTokens)
    //            {
    //                if (std::strncmp(token.c_str(), "(", 1) == 0)
    //                {
    //                    token.erase(0,1);
    //                    std::vector<std::string> stringNumbers;
    //                    boost::split(stringNumbers, token, boost::is_any_of("."));
    //                    mCondorClusterID = CommonLib::StringToInt(stringNumbers.front());
    //                    break;
    //                }
    //            }
    //        }
    //        if (mCondorClusterID != -1)
    //        {
    //            break;
    //        }
    //    }
    //    FILE_LOG(logINFO) << __FUNCTION_NAME__ << "Submitted to Condor cluster " << mCondorClusterID;
    //    std::cout << "Submitted to Condor cluster " << mCondorClusterID << std::endl;
    //    condorLog.close();
    //}

    ////______________________________________________________________________________________________________________

    //void GeneticAlgo::WaitForResults(std::deque<GenomePtr>& genomesToTest)
    //{
    //    zmq::socket_t resultsSocket(mZmqContext, ZMQ_REP);

    //    // this is required due to a bug in zeromq which causes the app to hang when the context is terminated
    //    int linger = 0;
    //    resultsSocket.setsockopt (ZMQ_LINGER, &linger, sizeof (linger));

    //    std::ostringstream s;
    //    s << "tcp://*:" <<  mGAPort;
    //    resultsSocket.bind(s.str().c_str());

    //    zmq::pollitem_t items [] = 
    //    {
    //        { resultsSocket, 0, ZMQ_POLLIN, 0 }
    //    };

    //    boost::posix_time::time_duration::sec_type timeOutPeriod = mTimeoutMinutes * 60;
    //    boost::posix_time::time_duration::sec_type secondsLeft = timeOutPeriod;
    //    std::size_t receivedCount = 0;
    //    std::size_t bailOutCount = genomesToTest.size(); //std::min(numberOfJobs, static_cast<std::size_t>(floor(0.95 * numberOfJobs)));

    //    FILE_LOG(logINFOwithCOUT) << __FUNCTION_NAME__ << "- Waiting for (" << bailOutCount << ") results for generation " << mGenerationNumber << 
    //        ". Num of jobs is " << genomesToTest.size() << ". Max wait time is " << 
    //        boost::posix_time::to_simple_string(boost::posix_time::time_duration(0, 0, secondsLeft));

    //    // NB when using ZeroMQ on Linux, boost::timer doesn't work
    //    boost::posix_time::ptime startTime(boost::posix_time::second_clock::local_time());
    //    while (1) 
    //    {
    //        zmq::poll(items, 1, static_cast<long>(secondsLeft * 1000));
    //        boost::posix_time::ptime currentTime(boost::posix_time::second_clock::local_time());
    //        boost::posix_time::time_duration timeDuration = currentTime - startTime;
    //        boost::posix_time::time_duration::sec_type elapsedSeconds = 
    //            (timeDuration.seconds() + (60 * timeDuration.minutes()) + (3600 * timeDuration.hours())); 
    //        secondsLeft = timeOutPeriod - elapsedSeconds;

    //        if (items[0].revents & ZMQ_POLLIN) 
    //        {
    //            zmq::message_t message;
    //            resultsSocket.recv(&message);
    //            char* receivedString = static_cast<char*>(malloc(message.size() + 1));
    //            memcpy(receivedString, message.data(), message.size());
    //            receivedString[message.size()] = 0;
    //            std::istringstream input(receivedString);

    //            // Send reply back to client
    //            zmq::message_t reply (4);
    //            memcpy ((void *) reply.data(), "_ok_", 4);
    //            resultsSocket.send (reply);

    //            boost::property_tree::ptree pt;
    //            boost::property_tree::xml_parser::read_xml(input, pt);
    //            boost::optional<GenomePtr> genome(AddCompleteGenomeToCache(genomesToTest, pt));
    //            
    //            SortPopulation();
    //            StoreState();

    //            receivedCount++;
    //            if (genome)
    //            {
    //                std::ostringstream s;
    //                s << mGenerationNumber <<"/" << receivedCount << "/" << bailOutCount << " " << 
    //                    (*genome)->ToString() << ". Time left is " << 
    //                    boost::posix_time::to_simple_string(boost::posix_time::time_duration(0, 0, secondsLeft)) << ".";
    //                std::cout << s.str() << std::endl;
    //                FILE_LOG(logINFO) << __FUNCTION_NAME__ << "" << s.str();
    //            }
    //            else
    //            {
    //                std::cout << "Genome not found! Received string: " << std::endl << input.str() << std::endl;
    //            }

    //            if (receivedCount == bailOutCount)
    //            {
    //                std::cout << "Received enough results (" << bailOutCount << ") for generation " << mGenerationNumber << std::endl;
    //                break;
    //            }
    //        }

    //        if (secondsLeft <= 0)
    //        {
    //            FILE_LOG(logINFO) << "Wait timeout. Recevived " << receivedCount << " genomes.";
    //            std::cout << "Timeout reached. Received " << receivedCount << " genomes. Proceeding to next generation." << std::endl;
    //            break;
    //        }
    //    }

    //    if (mCondorClusterID == -1)
    //    {
    //        return;
    //    }

    //    // kill any remaining jobs on the cluster - don't do this as we may have other instances of this GA
    //    
    //    s.str("");
    //    s << "condor_rm " << mCondorClusterID;
    //    FILE_LOG(logINFO) << "Removing jobs from cluster " << mCondorClusterID << " - executing command " << s.str();
    //    std::system(s.str().c_str());

    //    // print the best 20 results
    //    FILE_LOG(logINFO) << "***********************************";
    //    FILE_LOG(logINFO) << "Best " << mPrintBestNum << " results for generation " << mGenerationNumber;
    //    FILE_LOG(logINFO) << "***********************************";
    //    std::cout << std::endl << "***********************************" << std::endl;
    //    std::cout << "Best " << mPrintBestNum << " results for generation " << mGenerationNumber << std::endl;
    //    std::cout << "***********************************" << std::endl;
    //    std::size_t count = 0;
    //    BOOST_FOREACH(GenomePtr genome, mGenomeCache)
    //    {
    //        std::ostringstream s;
    //        s << genome->ToString();
    //        FILE_LOG(logINFO) << s.str();
    //        std::cout << s.str() << std::endl;
    //        count++;
    //        if (count == mPrintBestNum)
    //        {
    //            break;
    //        }
    //    }
    //    std::cout << std::endl;
    //}

    //______________________________________________________________________________________________________________

    /*boost::optional<GenomePtr> GeneticAlgo::AddCompleteGenomeToCache(std::deque<GenomePtr>& genomesToTest, const boost::property_tree::ptree& pt)
    {
        
        std::size_t genomeID = pt.get("results.genome-id", 0);

        BOOST_FOREACH(GenomePtr genome, genomesToTest)
        {
            if (genome->GetGenomeID() == genomeID)
            {
                genome->Update(pt);
                mGenomeCache.push_back(genome);
                return genome;
                break;
            }
        }

        FILE_LOG(logERROR) << __FUNCTION_NAME__ << "- Could not find genome " << genomeID;
        return boost::none;
    }*/

    //______________________________________________________________________________________________________________

    void GeneticAlgo::SendTestMessage(std::string machineName, std::string sendString)
    {
        try
        {
            zmq::socket_t sendSocket(mZmqContext, ZMQ_REQ);
            int linger = 0;
            sendSocket.setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
            std::ostringstream s;
            s << "tcp://" << machineName << ":55577"; 
            std::cout << "connecting to " << s.str() << std::endl;
            sendSocket.connect(s.str().c_str());

            zmq::message_t sendMessage(sendString.length());
            memcpy ((void *) sendMessage.data(), sendString.c_str(), sendString.length());
            zmq::pollitem_t items[1];
            items[0].events = ZMQ_POLLIN;
            items[0].socket = sendSocket;

            std::cout << "Sending test message to  " << s.str() << std::endl;

            if (!sendSocket.send(sendMessage))
            {
                std::cout << "Send failed" << std::endl;
            }
            else
            {
                zmq::poll(items, 1, 5000000);
                if (items[0].revents & ZMQ_POLLIN) 
                {
                    //std::cout << ReceiveString(sendSocket) << std::endl;
                }
                else
                {
                    std::cout << __FUNCTION_NAME__ << "- No response recieved." << std::endl;
                }
            }

        }
        catch (std::exception& e)
        {
            std::cout << "Exception: " << e.what() << std::endl;
        }
    }

    //______________________________________________________________________________________________________________

    void GeneticAlgo::StoreState(void) const
    {
        boost::property_tree::xml_writer_settings<char> settings(' ', 4);

        boost::property_tree::ptree ptCache;

        ptCache.put("state.population-size", mGenomeCache->size());
        ptCache.put("state.generation-number", mGenerationNumber); 
        BOOST_FOREACH(GenomePtr genome, *mGenomeCache)
        {     
            boost::property_tree::ptree genomeTree;
            genome->SaveAsXML(genomeTree);
            ptCache.add_child("state.genome", genomeTree);
        }

        boost::property_tree::xml_parser::write_xml(mCacheFile,  ptCache, std::locale(), settings);
    }

    //______________________________________________________________________________________________________________

    bool GeneticAlgo::RestoreState(void)
    {
        if (!boost::filesystem::exists(mCacheFile))
        {
            FILE_LOG(logINFO) << "No previous state found.";
            return false;
        }

        boost::property_tree::ptree cachePt;
        boost::property_tree::xml_parser::read_xml(mCacheFile, cachePt);
        mGenomeCache->clear();

        mGenerationNumber = cachePt.get("state.generation-number", 0);

        if (mGenerationNumber == 0)
        {
            mGenerationNumber = 1;
            FILE_LOG(logWARNING) << __FUNCTION_NAME__ << "Genome cache looks invalid. No generation number. Restarting from beginning";
            return false;
        }

        // don't overwrite any existing results
        ++mGenerationNumber;

        for (boost::property_tree::ptree::const_iterator itr = cachePt.get_child("state").begin(); itr != cachePt.get_child("state").end(); ++itr)
        {
            if (itr->first.compare("genome") == 0)
            {
                GenomePtr genome(boost::make_shared<Genome>());
                genome->SetIntParameterList(GetRandomGAParameters());
                genome->LoadFromXML(itr->second);
                mGenomeCache->push_back(genome);
            }
        }

        FILE_LOG(logINFOwithCOUT) << __FUNCTION_NAME__ << "Restored state. Generation number " << mGenerationNumber <<
            ". Loaded " << mGenomeCache->size() << " genomes from cache.";
        return true;
    }

    //______________________________________________________________________________________________________________

    void GeneticAlgo::SortPopulation()
    {
        std::sort(mGenomeCache->begin(), mGenomeCache->end(), CompareGenomeByObjective);
    }

    //______________________________________________________________________________________________________________

    std::string GeneticAlgo::GetExampleConfig(void)
    {
        std::ostringstream s;
        s <<
            "  <genetic-algo>" << std::endl <<
            "    <ga-server>tcp://wraith</ga-server>  <!-- Server where the GA is run. -->" << std::endl <<
            "    <ga-server-port>55566</ga-server-port>  <!-- TCP port on the machine where the GA is run. -->" << std::endl <<
            "    <genome-id>-1</genome-id>  <!-- Used internally. ID for the genome under test. -->" << std::endl <<
            "    <timeout-minutes>360</timeout-minutes>  <!-- Stop all backtests after this many minutes and" << std::endl <<
            "                                                 start the next generation. -->" << std::endl <<
            "    <population-size>20</population-size>  <!-- Number of genomes in a the population. -->" << std::endl <<
            "    <objective-function></objective-function>  <!-- The objective that we are optimising. One of:" << std::endl <<
            "                                                    pnl | sharpe | accuracy. -->" << std::endl <<
            "    <mutation-probability>10</mutation-probability>  <!-- Probability of a mutation in a genome. -->" << std::endl <<
            "    <num-breeders-percent>30</num-breeders-percent>  <!-- Top percentage of genomes in a population" << std::endl <<
            "                                                          to use as breeders for the next generation. -->" << std::endl <<
            "    <min-num-breeders>30</min-num-breeders>  <!-- The minimum number of genomes to use as breeders. -->" << std::endl <<
            "    <num-new-random-genomes>2</num-new-random-genomes>  <!-- Number of random genomes to create" << std::endl <<
            "                                                             for each generation. -->" << std::endl <<
            "    <num-generations>15</num-generations>  <!-- Stop after this many generations. -->" << std::endl <<
            "    <!-- The entries below are examples on how to define parameters for optimisation. -->" << std::endl <<
            "    <parameter id=\"stop-loss\" type=\"integer\" low=\"10\" high=\"200\" step=\"5\" />" << std::endl <<
            "    <parameter id=\"time-of-day\" type=\"categorical\" values=\"h1,h4,single,none\" />" << std::endl <<
            "    <parameter id=\"SVC-penalty\" type=\"exp-2\" low=\"1\" high=\"15\" step=\"1\" />" << std::endl <<
            "  </genetic-algo>";
        return s.str();
    }

    //______________________________________________________________________________________________________________
}
