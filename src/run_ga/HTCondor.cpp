#include "stdafx.hpp"
#include "HTCondor.hpp"

namespace GridGALib
{
	HTCondor::HTCondor(std::string filesLocation, zmq::context_t& zmqContext)
    :
        mGenerationNumber(0),
        mFilesLocation(filesLocation),
        mZmqContext(zmqContext),
        mTimeoutMinutes(1), 
        mPrintBestNum(20),
        //mExecutable("DeepThought"),
        mGetGenomeConfig(static_cast<GetGenomeConfigFunc>(0)),
        mStoreState(static_cast<StoreStateFunc>(0))
    {
        FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "Created HTCondor interface.";
        srand(static_cast<boost::uint32_t>(time(NULL)));
    }

    //______________________________________________________________________________________________________________

    HTCondor::~HTCondor(void)
    {
    }

    //______________________________________________________________________________________________________________

    bool HTCondor::ExecuteGeneration(GenomeList genomesToTest, GenomeList genomeCache, std::size_t generationNumber)
    {
        if (generationNumber == 0)
        {
            FILE_LOG(logERROR) << __FUNCTION_NAME__ << "Generation number has not been set!";
            return false;
        }
        mGenomesToTest = genomesToTest;
        mGenomeCache = genomeCache;
        mGenerationNumber = generationNumber;
        std::string submitFileName(WriteSubmitFile());
        SubmitToCluster(submitFileName);
        WaitForResults();
        return true;
    }

    //______________________________________________________________________________________________________________

    bool HTCondor::ReadConfig(boost::property_tree::ptree& pt)
    {
        mParamPrefix = CommonLib::GetOptionalParameter<std::string>("config.genetic-algo.param-prefix", pt, "--");
        mValuePrefix = CommonLib::GetOptionalParameter<std::string>("config.genetic-algo.value-prefix", pt, " ");
        mTimeoutMinutes = CommonLib::GetOptionalParameter<std::size_t>("config.htcondor.timeout-minutes", pt, 120);

        mServer = CommonLib::GetOptionalParameter<std::string>("config.htcondor.ga-server", pt, "tcp://localhost") + ":" +
            CommonLib::GetOptionalParameter<std::string>("config.htcondor.ga-server-port", pt, "55566");

        mGAPort = CommonLib::GetOptionalParameter<boost::int32_t>("config.htcondor.ga-server-port", pt, 55566);

        mExecutable = CommonLib::GetOptionalParameter<std::string>("config.genetic-algo.executable", pt, "not-set");
        mExtractObj = CommonLib::GetOptionalParameter<std::string>("config.genetic-algo.extract-obj", pt, "not-set");

        mArguments = CommonLib::GetOptionalParameter<std::string>("config.genetic-algo.arguments", pt, "not-set");

        for (boost::property_tree::ptree::const_iterator itr=pt.get_child("config.genetic-algo").begin(); itr!=pt.get_child("config.genetic-algo").end(); ++itr)
        {
            if (itr->first.compare("required-file") == 0)
            {
                std::string file(itr->second.get_value("required-file"));
                file = mFilesLocation + "/" + file;
                mFiles.push_back(file);
            }
        }

        // create a batch script to clean up
        std::string batFile = mFilesLocation + "/del_ga.bat";
        std::ofstream fileOut(batFile.c_str());
        fileOut <<
            "@echo off\n"
            "\n"
            "SET /P yesno=Cleanup all generated files related to the genetic algorithm?[Y/N]:\n"
            "IF \"%yesno%\"==\"y\" GOTO DeleteFiles\n"
            "IF \"%yesno%\"==\"Y\" GOTO DeleteFiles\n"
            "GOTO End\n"
            "\n"
            ":DeleteFiles\n"
            "for /D %%f in (generation-*) do if exist %%f rmdir /s /q %%f\n"
            "\n"
            "if exist generation* del generation*\n"
            "if exist genetic-algo-cache.xml del genetic-algo-cache.xml\n"
            "\n"
            ":End\n";
        fileOut.close();

        return true;
    }

    //______________________________________________________________________________________________________________

    std::string HTCondor::WriteSubmitFile(void) 
    {
        std::ostringstream s;
        s << mFilesLocation << "/generation-" << mGenerationNumber << ".submit";
        std::ofstream submitFile(s.str().c_str());

#ifdef _WIN32
        submitFile << "Executable = htcondor_job_wrapper.exe" << "\n";
#else
        submitFile << "Executable = htcondor_job_wrapper" << "\n";
#endif

        
        submitFile << "Universe = vanilla\n";
        submitFile << "should_transfer_files = yes\n";
        submitFile << "stream_error = false\n";
        submitFile << "stream_input = false\n";
        submitFile << "stream_output = false\n";
        submitFile << "should_transfer_files = YES\n";
        submitFile << "when_to_transfer_output = ON_EXIT_OR_EVICT\n";    

        std::ostringstream logFileName;
        logFileName << "genetic-algo.condor." << mGenerationNumber << ".log";
        try
        {
            if (boost::filesystem::exists(logFileName.str()))
            {       
                boost::filesystem::remove(logFileName.str());
            }
        }
        catch (std::exception& e)
        {
            FILE_LOG(logERROR) << __FUNCTION_NAME__ << "" << e.what();
            std::cerr << __FUNCTION_NAME__ << "" << e.what() << std::endl;
            exit(-1);
        }

        submitFile << "log = " << logFileName.str() << "\n";

        std::ostringstream generationSubDir;
        generationSubDir << mFilesLocation << "/generation-" << mGenerationNumber;

        if (boost::filesystem::exists(generationSubDir.str()))
        {
            FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "- Emptying directory " << generationSubDir.str() << ".";
            try
            {
                boost::filesystem::remove_all(generationSubDir.str());
                // Windows returns an access denied if we immediately attempt to recreate the directory hence the
                // reason for this silly code.
                bool fail = true;
                while (fail)
                {
                    try
                    {
                        fail = false;
                        boost::filesystem::create_directory(generationSubDir.str());
                    }
                    catch (std::exception& e)
                    {
                        FILE_LOG(logERROR) << __FUNCTION_NAME__ << "" << e.what();
                        fail =true;
                    }
                }
            }
            catch (std::exception& e)
            {
                FILE_LOG(logERROR) << __FUNCTION_NAME__ << "- " << e.what();
                exit(-1);
            }
            FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "- " << generationSubDir.str() << " has been deleted.";
        }

        if (!boost::filesystem::exists(generationSubDir.str()))
        {
            FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "- Creating directory " << generationSubDir.str();
            boost::filesystem::create_directory(generationSubDir.str());
            FILE_LOG(logDEBUG) << __FUNCTION_NAME__ << "- " << generationSubDir.str() << " has been created.";
        }

        BOOST_FOREACH(GenomePtr genome, *mGenomesToTest)
        {
            if (!genome->IsComplete())
            {  
                std::ostringstream outFileName;
                outFileName << genome->GetGenomeID();

                std::ostringstream transferInputFiles;

                if (mGetGenomeConfig)
                {
                    transferInputFiles <<  mGetGenomeConfig(genome, generationSubDir.str());
                    if (transferInputFiles.str().size() > 0)
                    {
                        transferInputFiles << ",";
                    }
                }

                BOOST_FOREACH(std::string s, mFiles)
                {
                    transferInputFiles << s << ",";
                }

                std::string arguments = mArguments;
                boost::replace_all(arguments, "%GA%", genome->GetCommandLineArguments(mParamPrefix, mValuePrefix));
                submitFile << "Arguments = " << genome->GetGenomeID() << "_obj_test_config.xml" << "\n";
                submitFile << "Output = " << generationSubDir.str() << "/" << outFileName.str() << ".out\n";
                submitFile << "Error = " << generationSubDir.str() << "/" << outFileName.str() << ".err\n";

                std::ostringstream s;
                s << 
                    "<config>" << std::endl <<
                    "   <execute>" << mExecutable << " " << arguments << "</execute>" << std::endl;
                if (!boost::iequals(mExtractObj,"not-set"))
                {
                    s <<
                        "   <extract-obj-value>" << mExtractObj << "</extract-obj-value>" << std::endl;
                }
                s <<
                    "   <server>" << mServer << "</server>" << std::endl <<
                    "   <genome-id>" << genome->GetGenomeID() << "</genome-id>" << std::endl <<
                    "</config>";
                std::ostringstream jobConfigFileName;
                jobConfigFileName << generationSubDir.str() << "/" << genome->GetGenomeID() << "_obj_test_config.xml";
                std::ofstream jobConfig(jobConfigFileName.str().c_str());
                jobConfig << s.str();
                jobConfig.close();
 
#ifdef _WIN32
                submitFile << "transfer_input_files = htcondor_job_wrapper.exe," << jobConfigFileName.str() << "," << transferInputFiles.str() << "\n"; // << GetPythonFiles();
                submitFile << "Requirements   = (OpSys == \"WINDOWS\" && Arch ==\"X86_64\") || (OpSys == \"WINDOWS\" && Arch ==\"INTEL\")\n";
#else
                submitFile << "transfer_input_files = htcondor_job_wrapper," << jobConfigFileName.str() << "," << transferInputFiles.str() << "\n"; ;
#endif
                //submitFile << "transfer_output_files = results.zip\n";

                // copy into the 'generation' directory
                //submitFile << "transfer_output_remaps = \"results.zip = " << generationSubDir.str() << "/" << genome->GetGenomeID() << ".results.zip\"\n";
                submitFile << "Queue\n";
            }
        }

        submitFile.close();
        return s.str();
    }

    //______________________________________________________________________________________________________________

    //std::string HTCondor::GetPythonFiles(void)
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

    //______________________________________________________________________________________________________________

    void HTCondor::SubmitToCluster(const std::string& submitFileName)
    {
        std::cerr << __FUNCTION_NAME__ << std::endl;
        std::ostringstream cmd;
        cmd << "condor_submit " << submitFileName;
        FILE_LOG(logINFO) << "Executing " << cmd.str();
        std::system(cmd.str().c_str());
        
        // parse the log and get the cluster id
        std::ostringstream condorLogFile;
        condorLogFile << "genetic-algo.condor." << mGenerationNumber << ".log";
        std::ifstream condorLog(condorLogFile.str().c_str());
        std::string line;
        while ( condorLog.good() )
        {
            getline(condorLog,line);
            boost::trim(line);
            if (!line.empty())
            {
                std::vector<std::string> stringTokens;
                boost::split(stringTokens, line, boost::is_any_of(" "));
                BOOST_FOREACH(std::string& token, stringTokens)
                {
                    if (std::strncmp(token.c_str(), "(", 1) == 0)
                    {
                        token.erase(0,1);
                        std::vector<std::string> stringNumbers;
                        boost::split(stringNumbers, token, boost::is_any_of("."));
                        mCondorClusterID = CommonLib::StringToInt(stringNumbers.front());
                        break;
                    }
                }
            }
            if (mCondorClusterID != -1)
            {
                break;
            }
        }
        FILE_LOG(logINFO) << __FUNCTION_NAME__ << "Submitted to Condor cluster " << mCondorClusterID;
        std::cout << "Submitted to Condor cluster " << mCondorClusterID << std::endl;
        condorLog.close();
    }

    //______________________________________________________________________________________________________________

    void HTCondor::SortPopulation(void)
    {
        std::sort(mGenomeCache->begin(), mGenomeCache->end(), CompareGenomeByObjective);
    }

    //______________________________________________________________________________________________________________

    void HTCondor::WaitForResults(void)
    {
        zmq::socket_t resultsSocket(mZmqContext, ZMQ_REP);

        // this is required due to a bug in zeromq which causes the app to hang when the context is terminated
        int linger = 0;
        resultsSocket.setsockopt (ZMQ_LINGER, &linger, sizeof (linger));

        std::ostringstream s;
        s << "tcp://*:" <<  mGAPort;
        resultsSocket.bind(s.str().c_str());

        zmq::pollitem_t items [] = 
        {
            { resultsSocket, 0, ZMQ_POLLIN, 0 }
        };

        boost::posix_time::time_duration::sec_type timeOutPeriod = mTimeoutMinutes * 60;
        boost::posix_time::time_duration::sec_type secondsLeft = timeOutPeriod;
        std::size_t receivedCount = 0;
        std::size_t bailOutCount = mGenomesToTest->size(); //std::min(numberOfJobs, static_cast<std::size_t>(floor(0.95 * numberOfJobs)));

        std::cout << "Waiting for (" << bailOutCount << ") results on port " << mGAPort << " for generation " << mGenerationNumber << 
            ". Num of jobs is " << mGenomesToTest->size() << ". Max wait time is " << 
            boost::posix_time::to_simple_string(boost::posix_time::time_duration(0, 0, secondsLeft)) << std::endl;

        // NB when using ZeroMQ on Linux, boost::timer doesn't work
        boost::posix_time::ptime startTime(boost::posix_time::second_clock::local_time());
        while (1) 
        {
            zmq::poll(items, 1, static_cast<long>(secondsLeft * 1000));
            boost::posix_time::ptime currentTime(boost::posix_time::second_clock::local_time());
            boost::posix_time::time_duration timeDuration = currentTime - startTime;
            boost::posix_time::time_duration::sec_type elapsedSeconds = 
                (timeDuration.seconds() + (60 * timeDuration.minutes()) + (3600 * timeDuration.hours())); 
            secondsLeft = timeOutPeriod - elapsedSeconds;

            if (items[0].revents & ZMQ_POLLIN) 
            {
                zmq::message_t message;
                resultsSocket.recv(&message);
                char* receivedString = static_cast<char*>(malloc(message.size() + 1));
                memcpy(receivedString, message.data(), message.size());
                receivedString[message.size()] = 0;
                std::istringstream input(receivedString);

                // Send reply back to client
                zmq::message_t reply (4);
                memcpy ((void *) reply.data(), "_ok_", 4);
                resultsSocket.send (reply);

                boost::property_tree::ptree pt;
                boost::property_tree::xml_parser::read_xml(input, pt);
                GenomePtr genome(AddCompleteGenomeToCache(pt));
                
                SortPopulation();
                //StoreState();
                if (mStoreState)
                {
                    mStoreState;
                }

                receivedCount++;
                if (genome)
                {
                    std::ostringstream s;
                    s << mGenerationNumber <<"/" << receivedCount << "/" << bailOutCount << " " << 
                        genome->ToString() << ". Time left is " << 
                        boost::posix_time::to_simple_string(boost::posix_time::time_duration(0, 0, secondsLeft)) << ".";
                    std::cout << s.str() << std::endl;
                    FILE_LOG(logINFO) << __FUNCTION_NAME__ << "" << s.str();
                }
                else
                {
                    std::cout << "Genome not found! Received string: " << std::endl << input.str() << std::endl;
                }

                if (receivedCount == bailOutCount)
                {
                    std::cout << "Received enough results (" << bailOutCount << ") for generation " << mGenerationNumber << std::endl;
                    break;
                }
            }

            if (secondsLeft <= 0)
            {
                FILE_LOG(logINFO) << "Wait timeout. Recevived " << receivedCount << " genomes.";
                std::cout << "Timeout reached. Received " << receivedCount << " genomes. Proceeding to next generation." << std::endl;
                break;
            }
        }

        if (mCondorClusterID == -1)
        {
            return;
        }

        // kill any remaining jobs on the cluster - don't do this as we may have other instances of this GA
        
        s.str("");
        s << "condor_rm " << mCondorClusterID;
        FILE_LOG(logINFO) << "Removing jobs from cluster " << mCondorClusterID << " - executing command " << s.str();
        std::system(s.str().c_str());

        // print the best 20 results
        FILE_LOG(logINFO) << "***********************************";
        FILE_LOG(logINFO) << "Best " << mPrintBestNum << " results for generation " << mGenerationNumber;
        FILE_LOG(logINFO) << "***********************************";
        std::cout << std::endl << "***********************************" << std::endl;
        std::cout << "Best " << mPrintBestNum << " results for generation " << mGenerationNumber << std::endl;
        std::cout << "***********************************" << std::endl;
        std::size_t count = 0;
        BOOST_FOREACH(GenomePtr genome, *mGenomeCache)
        {
            std::ostringstream s;
            s << genome->ToString();
            FILE_LOG(logINFO) << s.str();
            std::cout << s.str() << std::endl;
            count++;
            if (count == mPrintBestNum)
            {
                break;
            }
        }
        std::cout << std::endl;
    }

    //______________________________________________________________________________________________________________

    GenomePtr HTCondor::AddCompleteGenomeToCache(const boost::property_tree::ptree& pt)
    {
        
        std::size_t genomeID = pt.get("results.id", 0);

        BOOST_FOREACH(GenomePtr genome, *mGenomesToTest)
        {
            if (genome->GetGenomeID() == genomeID)
            {
                genome->Update(pt);
                mGenomeCache->push_back(genome);
                return genome;
                break;
            }
        }

        std::cerr << "- Could not find genome " << genomeID;
        return boost::shared_ptr<Genome>();
    }

    //______________________________________________________________________________________________________________

    void HTCondor::SendTestMessage(std::string machineName, std::string sendString)
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

    //void HTCondor::StoreState(void) const
    //{
    //    boost::property_tree::xml_writer_settings<char> settings(' ', 4);

    //    boost::property_tree::ptree ptCache;

    //    ptCache.put("state.population-size", (*mGenomeCache).size());
    //    ptCache.put("state.generation-number", mGenerationNumber); 
    //    BOOST_FOREACH(GenomePtr genome, mGenomeCache)
    //    {     
    //        boost::property_tree::ptree genomeTree;
    //        genome->SaveAsXML(genomeTree);
    //        ptCache.add_child("state.genome", genomeTree);
    //    }

    //    boost::property_tree::xml_parser::write_xml(mCacheFile, ptCache, std::locale(), settings);
    //}

    ////______________________________________________________________________________________________________________

    //bool HTCondor::RestoreState(void)
    //{
    //    if (!boost::filesystem::exists(mCacheFile))
    //    {
    //        FILE_LOG(logINFO) << "No previous state found.";
    //        return false;
    //    }

    //    boost::property_tree::ptree cachePt;
    //    boost::property_tree::xml_parser::read_xml(mCacheFile, cachePt);
    //    mGenomeCache.clear();

    //    mGenerationNumber = cachePt.get("state.generation-number", 0);

    //    if (mGenerationNumber == 0)
    //    {
    //        mGenerationNumber = 1;
    //        FILE_LOG(logWARNING) << __FUNCTION_NAME__ << "Genome cache looks invalid. No generation number. Restarting from beginning";
    //        return false;
    //    }

    //    // don't overwrite any existing results
    //    ++mGenerationNumber;

    //    for (boost::property_tree::ptree::const_iterator itr = cachePt.get_child("state").begin(); itr != cachePt.get_child("state").end(); ++itr)
    //    {
    //        if (itr->first.compare("genome") == 0)
    //        {
    //            GenomePtr genome(boost::make_shared<Genome>(mObjectiveFunction));
    //            genome->SetIntParameterList(GetRandomGAParameters());
    //            genome->LoadFromXML(itr->second);
    //            mGenomeCache.push_back(genome);
    //        }
    //    }

    //    FILE_LOG(logINFOwithCOUT) << __FUNCTION_NAME__ << "Restored state. Generation number " << mGenerationNumber <<
    //        ". Loaded " << mGenomeCache.size() << " genomes from cache.";
    //    return true;
    //}

    //______________________________________________________________________________________________________________

    //bool CompareGenomeByObjective(GenomePtr i, GenomePtr j)
    //{
    //    return (i->GetObjective() > j->GetObjective());
    //}

    ////______________________________________________________________________________________________________________

    //void HTCondor::SortPopulation()
    //{
    //    std::sort(mGenomeCache.begin(), mGenomeCache.end(), CompareGenomeByObjective);
    //}

    //______________________________________________________________________________________________________________

    /*std::string HTCondor::GetExampleConfig(void)
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
    }*/

    //______________________________________________________________________________________________________________
}
