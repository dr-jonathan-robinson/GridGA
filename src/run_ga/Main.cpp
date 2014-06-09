#include "stdafx.hpp"

#include "GeneticAlgo.hpp"
#include "VersionConfig.hpp"

namespace po = boost::program_options;

//______________________________________________________________________________________________________________

struct ArgTypeInt
    : public boost::program_options::typed_value<int>
{
    ArgTypeInt(const std::string& name)
        : boost::program_options::typed_value<int>(&my_value)
        , my_name(name)
        , my_value(0)
    {
    }
    std::string name() const { return my_name; }
    std::string my_name;
    int my_value;
};

//______________________________________________________________________________________________________________

struct ArgTypeString
    : public boost::program_options::typed_value<std::string>
{
    ArgTypeString(const std::string& name)
        : boost::program_options::typed_value<std::string>(&my_value)
        , my_name(name)
        , my_value("")
    {
    }
    std::string name() const { return my_name; }
    std::string my_name;
    std::string my_value;
};

//______________________________________________________________________________________________________________

struct ArgTypeDouble
    : public boost::program_options::typed_value<double>
{
    ArgTypeDouble(const std::string& name)
        : boost::program_options::typed_value<double>(&my_value)
        , my_name(name)
        , my_value(0.0)
    {
    }
    std::string name() const { return my_name; }
    std::string my_name;
    double my_value;
};

//______________________________________________________________________________________________________________

int main(int argc, char* argv[])
{
	po::options_description desc("Usage");

	desc.add_options()
		("execute", new ArgTypeString(std::string("<directory>")), 
          "Backtest the configuration file in <directory>. Various files are output to the same location. The configuration file must be named config.xml or _config.xml.")

        ("genetic-algo", new ArgTypeString(std::string("<config template>")), 
            "Run a genetic algo using the supplied file as the template. Requires an HTCondor cluster.")

        //   ("tcp-port", new ArgTypeInt(std::string("<Port Number>")),
     //       "Used with --genetic-algo. Specifies the TCP port node comminicate with the GA server on.")
      //  ("test-send", "Send a test message to another DeepThought instance via ZeroMQ to test comms.")
      //  ("test-receive", "Test receiving messages via ZeroMQ to test comms.")
        
      ("version", "Print version info.")

	;

	po::variables_map variablesMap;
	try
	{
        namespace po_style = boost::program_options::command_line_style;
        po::store(po::command_line_parser(argc, argv).options(desc).style(po_style::unix_style|po_style::case_insensitive).run(), variablesMap);
	}
	catch (std::exception &e)
	{
		std::cout << "Error parsing command line : " << e.what() << "\n\n" << desc << "\n";
		return 1;
	}

	po::notify(variablesMap);

    if (variablesMap.count("version"))
    {
        std::cout << "GridGA Version :\t\t" << DEEPTHOUGHT_VERSION << std::endl <<
            "Build type :\t\t\t" << BUILD_TYPE << std::endl <<
            "Build host :\t\t\t" << COMPILE_HOST << std::endl <<
            "Compiler : \t\t\t" << COMPILER_ID << " " << COMPILER_VERSION << std::endl <<
            "Boost version : \t\t" << BOOST_COMPILE_VERSION << std::endl <<
            "Home directory : \t\t" << DeepThoughtLib::FileUtils::GetHomeDir() << std::endl <<
            "Binary install directory : \t" << DeepThoughtLib::FileUtils::GetExePath() << std::endl;
        return 1;
    }


    std::string logFileName("ga.log");

    if (variablesMap.count("execute"))
    {
        std::string	filesLocation(variablesMap["execute"].as<std::string>());
        logFileName = filesLocation + "/ga_execute.log";
    }

    
    if (variablesMap.count("genetic-algo"))
    {
        std::string filesLocation(variablesMap["genetic-algo"].as<std::string>());
        logFileName = filesLocation + "/genetic-algo.log";
    }

    Logger::Initialise(logFileName);

	if (variablesMap.count("help"))
	{
		std::cout << desc << std::endl;
		return 0;
	}


    if (variablesMap.count("genetic-algo"))
    {
        zmq::context_t zmqContext(1);
        GridGALib::GeneticAlgo geneticAlgo(variablesMap["genetic-algo"].as<std::string>(), zmqContext);
        if (!geneticAlgo.ReadConfig())
        {
            return 1;
        }        
        geneticAlgo.Evolve();
        return 0;
    }

    //if (variablesMap.count("test-send"))
    //{
    //    DeepThoughtLib::AppContext appContext;
    //    appContext.TransmitTestData();
    //    std::cerr << "finished" << std::endl;
    //    return 0;
    //}    

    //if (variablesMap.count("test-receive"))
    //{
    //    zmq::context_t zmqContext(1);
    //    DeepThoughtLib::GeneticAlgo geneticAlgo(false, "", zmqContext);
    //    std::deque<DeepThoughtLib::GenomePtr> genomesToTest;
    //    geneticAlgo.WaitForResults(genomesToTest);
    //    return 0;
    //}

	if (variablesMap.count("execute"))
	{
        std::string filesLocation = variablesMap["execute"].as<std::string>();

        if (!boost::filesystem::exists(filesLocation))
        {
            std::cerr << "Cannot find location \"" << filesLocation << "\"" << std::endl;
            return -1;
        }

        //DeepThoughtLib::AppContext appContext;
        //appContext.Backtest(filesLocation);

		return 0;
	}

	std::cout << desc << std::endl;

	return 0;
}

