#include "stdafx.hpp"

#include "GenerateXMLConfig.hpp"

namespace GridGALib
{
	GenerateXMLConfig::GenerateXMLConfig()
    {
    }

    //______________________________________________________________________________________________________________

    GenerateXMLConfig::~GenerateXMLConfig(void)
    {
    }

    //______________________________________________________________________________________________________________

    void GenerateXMLConfig::ParseConfigBlocks(boost::property_tree::ptree& pt, const GAParameterMapPtr parameterMap)
    {
        // recursively iterate the XML config file
        BOOST_FOREACH(boost::property_tree::ptree::value_type& configBlock, pt)
        {
            ParseConfigBlocks(configBlock.second, parameterMap);
            if (configBlock.second.get_child_optional("<xmlattr>.ga-subst"))
            {
                BOOST_FOREACH(GAParameterMap::value_type& parameter, *parameterMap)
                {
                    if (boost::iequals(configBlock.second.get_child("<xmlattr>").get<std::string>("ga-subst"), parameter.second->GetIdentifier()))
                    {
                        configBlock.second.put_value<std::string>(parameter.second->GetValueForConfig());
                    }
                }
            }
        }
    }

    //______________________________________________________________________________________________________________

    std::string GenerateXMLConfig::GetConfigForGA(const GenomePtr genome, const std::string& dir)
    {
        if (!boost::filesystem::exists(mConfigXMLTemplateFileName))
        {
            FILE_LOG(logERROR) << __FUNCTION_NAME__ << "- Cannot find template file "  << mConfigXMLTemplateFileName;
        }

        // read in the file line by line to remove spaces as the read_xml directly from file fucks up the output.
        std::ifstream inFile(mConfigXMLTemplateFileName.c_str());
        std::ostringstream s;
        std::string line;
        while ( inFile.good() )
        {
            getline(inFile,line);
            boost::trim(line);
            if (!line.empty())
            {
                s << line;
            }
        }
        inFile.close();

        boost::property_tree::ptree pt;
        std::istringstream input(s.str());
        read_xml(input, pt);

        ParseConfigBlocks(pt, genome->GetParameters());

        pt.put("config.genetic-algo.genome-id", genome->GetGenomeID());

        std::ostringstream output;  
        boost::property_tree::xml_writer_settings<char> settings(' ', 4);
        boost::property_tree::xml_parser::write_xml(output, pt, settings);
        return output.str();
    }

    //______________________________________________________________________________________________________________

}
