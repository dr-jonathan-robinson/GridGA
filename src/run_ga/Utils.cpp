#include "stdafx.hpp"

namespace CommonLib
{
    bool GetOptionalBoolParameter(std::string paramName, const boost::property_tree::ptree& pt, bool defaultBool)
    {    
        boost::optional< const boost::property_tree::ptree& > child = pt.get_child_optional(paramName);
        if( !child )
        {
            FILE_LOG(logWARNING) << __FUNCTION_NAME__ << "" << paramName << " not found, using default of " << defaultBool;
            return defaultBool;
        }
        return boost::iequals(GetOptionalParameter<std::string>(paramName, pt, "true"), "true");
    }

    //______________________________________________________________________________________________________________

}