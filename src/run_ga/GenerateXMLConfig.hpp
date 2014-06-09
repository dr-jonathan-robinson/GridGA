#pragma once

#include "stdafx.hpp"

#include "Genome.hpp"
#include "GenerateXMLConfig.hpp"

namespace GridGALib
{
	class GenerateXMLConfig
    {
    public:
        GenerateXMLConfig(); //std::string configTemplateFileName, zmq::context_t& zmqContext);
        ~GenerateXMLConfig(void);
        std::string GetConfigForGA(const GenomePtr genome, const std::string& dir);

        //void ReadConfig(void);
        //void Evolve(void);
        //void SendTestMessage(std::string machineName, std::string sendString);

        //GAParameterMapPtr GetRandomGAParameters();

        //void WaitForResults(std::deque<GenomePtr>& genomesToTest); 
        //static std::string GetExampleConfig(void);
    private:
        /*std::deque<GenomePtr> mGenomeCache;
        std::size_t mPopulationSize;
        double mNumBreedersPercent;
        std::size_t mMinNumBreeders;
        std::size_t mMutationProbability; 
        std::size_t mNumNewRandomGenomes;
        std::size_t mGenerationNumber;       
        std::size_t mNumGenerations;
        boost::int32_t mCondorClusterID;
        GAParameterMapPtr mParameterMap;
        std::string mFilesLocation;
        boost::int32_t mGAPort;
        zmq::context_t& mZmqContext;
        std::size_t mTimeoutMinutes;
        std::size_t mPrintBestNum;
        bool mUsingRecordedSignals;
        std::string mCacheFile;
        ObjectiveFunction mObjectiveFunction;
        CrossFunc mCross;
        std::string mExecutable;
        GetGenomeConfigFunc mGetGenomeConfig;*/
        std::string mConfigXMLTemplateFileName;


        void ParseConfigBlocks(boost::property_tree::ptree& pt, const GAParameterMapPtr parameterMap);
        /*bool GAParametersOk(const GAParameterMapPtr parameters);
        GenomePtr CreateRandomGenome(void);
        void CrossBySlicing(const GenomePtr parent1, const GenomePtr parent2, GenomePtr child1, GenomePtr child2);
        void CrossBySwap(const GenomePtr parent1, const GenomePtr parent2, GenomePtr child1, GenomePtr child2);
        bool AddGenomeToPopulation(std::deque<GenomePtr>& genomesToTest, GenomePtr genome);
        void RemoveIncomleteGenomes(void);
        std::deque<GenomePtr> NextGeneration(void);
        std::string WriteSubmitFile(const std::deque<GenomePtr>& genomePopulation);
        std::string GetPythonFiles(void);
        void SubmitToCluster(const std::string& submitFileName);
        void SendString(void* socket, const std::string& sendString) const; 
        void StoreState(void) const;
        bool RestoreState(void);   
        boost::optional<GenomePtr> AddCompleteGenomeToCache(std::deque<GenomePtr>& genomesToTest, const boost::property_tree::ptree& pt);
        void SortPopulation();*/     
    };
}