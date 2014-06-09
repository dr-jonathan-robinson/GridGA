#pragma once

#include "stdafx.hpp"

#include "GenerateXMLConfig.hpp"
#include "Genome.hpp"

namespace GridGALib
{
    typedef boost::function<std::string (const GenomePtr genome, const std::string& dir)> GetGenomeConfigFunc;
    typedef boost::function<void (void)> StoreStateFunc;

	class HTCondor
    {
    public:
        HTCondor(std::string configTemplateFileName, zmq::context_t& zmqContext);
        ~HTCondor(void);
        bool ReadConfig(boost::property_tree::ptree& pt);
        bool ExecuteGeneration(GenomeList genomesToTest, GenomeList genomeCache, std::size_t generationNumber);
    private:
        std::size_t mGenerationNumber;       
        std::size_t mNumGenerations;
        boost::int32_t mCondorClusterID;
        //GAParameterMapPtr mParameterMap;
        std::string mFilesLocation;
        boost::int32_t mGAPort;
        zmq::context_t& mZmqContext;
        std::size_t mTimeoutMinutes;
        std::size_t mPrintBestNum;
        GetGenomeConfigFunc mGetGenomeConfig;
        std::string mExecutable;
        std::string mExtractObj;
        //std::string mFullPathExecutable;
        std::string mArguments;
        std::string mConfigXMLTemplateFileName;
        GenomeList mGenomesToTest;
        GenomeList mGenomeCache;
        GenerateXMLConfig mGenerateXMLConfig;
        StoreStateFunc mStoreState;
        std::string mParamPrefix;
        std::string mValuePrefix;
        std::string mServer;
        std::vector<std::string> mFiles;

        std::string WriteSubmitFile(void);
        //std::string GetPythonFiles(void);
        void SortPopulation(void);
        void SubmitToCluster(const std::string& submitFileName);
        void SendTestMessage(std::string machineName, std::string sendString);
        void SendString(void* socket, const std::string& sendString) const; 
        //void StoreState(void) const;
        //bool RestoreState(void); 
        void WaitForResults(void);
        GenomePtr AddCompleteGenomeToCache(const boost::property_tree::ptree& pt);
    };
}