#pragma once

#include "Log.hpp"
//#include "zmq.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <stdio.h>
#include <iostream>
#include <string>
#include <cmath>

#ifndef _WIN32 
#   define __FUNCTION_NAME__ "[" << __func__ << "] "
#else
#   define __FUNCTION_NAME__ "[" << __FUNCTION__ << "] "
#endif

//#define DO_TIMERS

#ifdef DO_TIMERS
#    define START_TIMER boost::posix_time::ptime __startTime(boost::posix_time::microsec_clock::local_time());
#    define STOP_TIMER(__what__) boost::posix_time::ptime __endTime(boost::posix_time::microsec_clock::local_time()); \
    boost::posix_time::time_duration timeDuration = __endTime - __startTime; \
    FILE_LOG(logINFO) << __FUNCTION_NAME__ << "" << __what__ << " took " << boost::posix_time::to_simple_string(timeDuration) << ".";
#else
#    define START_TIMER
#    define STOP_TIMER(_)
#endif

namespace CommonLib
{
    inline double StringToDouble(std::string& inStr) 
    {
        const char *p = inStr.c_str();
        double r = 0.0;
        bool neg = false;
        if (*p == '-') {
            neg = true;
            ++p;
        }
        while (*p >= '0' && *p <= '9') {
            r = (r*10.0) + (*p - '0');
            ++p;
        }
        if (*p == '.') {
            double f = 0.0;
            int n = 0;
            ++p;
            while (*p >= '0' && *p <= '9') {
                f = (f*10.0) + (*p - '0');
                ++p;
                ++n;
            }
            r += f / std::pow(10.0, n);
        }
        if (neg) {
            r = -r;
        }
        return r;
    }


    // from http://tinodidriksen.com/uploads/code/cpp/speed-string-to-int.cpp
    inline boost::int32_t StringToInt(std::string& inStr)
    {
        const char* p = inStr.c_str();
        int x = 0;
        bool neg = false;
        if (*p == '-') {
            neg = true;
            ++p;
        }
        while (*p >= '0' && *p <= '9') {
            x = (x*10) + (*p - '0');
            ++p;
        }
        if (neg) {
            x = -x;
        }
        return x;
    }


    inline bool DoublesGreaterThanOrEqual(double first, double second)
    {
        return ((first + 0.00000001) >= second);
    }


    inline bool DoublesLessThanOrEqual(double first, double second)
    {
        return ((first - 0.00000001) <= second);
    }


    inline bool DoublesEqual(double first, double second)
    {
        return (std::abs(first - second) < 0.00000001);
    }


    inline std::string GetBaseFilename(std::string fileName) 
    {
        return boost::filesystem::path(fileName).stem().string();
    }

    inline std::string GetDirectoryFromFileName(std::string fileName)
    {
        boost::filesystem::path p(fileName);
        return p.parent_path().string();
    }

    inline std::string GetConfigFileNameIfExists(std::string filesLocation)
    {
        std::string configFileName = filesLocation + "/_config.xml";
        if (!boost::filesystem::exists(configFileName))
        {
            configFileName = filesLocation + "/config.xml";
        }

        if (boost::iequals(filesLocation, "."))
        {
            // find the first matching xml file in the directory
            std::string filePrefix = boost::filesystem::path(configFileName).stem().string();

            const std::string target_path( "." );
            const boost::regex my_filter( filePrefix+".*\\.xml" );

            boost::filesystem::directory_iterator end_itr; // Default ctor yields past-the-end
            for( boost::filesystem::directory_iterator i( target_path ); i != end_itr; ++i )
            {
                // Skip if not a file
                if( !boost::filesystem::is_regular_file( i->status() ) ) continue;

                boost::smatch what;

                // Skip if no match
                if( !boost::regex_match( i->path().filename().string(), what, my_filter ) ) continue;

                // File matches, store it
                configFileName = "./" + i->path().filename().string();
                FILE_LOG(logINFO) << "Found config file " << configFileName;
                break;
            }
        }

        if (!boost::filesystem::exists(configFileName))
        {
            FILE_LOG(logERROR) << __FUNCTION_NAME__ << "Cannot find config in directory " << filesLocation << ". Check that config.xml or _config.xml exists."; 
            return "";
        }
        return configFileName;
    }

    bool ValidTime(boost::posix_time::ptime time, std::vector<boost::int32_t> hours, std::vector<boost::int32_t> days);

    boost::posix_time::time_duration GetOpenTimeOfDay();
    boost::int32_t DayOfWeekToInt(std::string s);

    template<typename T>
    std::string SomethingToString(const T& val)
    {
        std::ostringstream s;
        s << val;
        return s.str();
    }

    template<typename T>
    T GetOptionalParameter(std::string paramName, const boost::property_tree::ptree& pt, const T& defaultVal)
    {
        try
        {
            boost::optional< const boost::property_tree::ptree& > child = pt.get_child_optional(paramName);
            if( !child )
            {
                FILE_LOG(logWARNING) << __FUNCTION_NAME__ << "Value of parameter " << paramName << " not given, using default of " << defaultVal;
                return defaultVal;
            }
            return (pt.get<T>(paramName));
        }
        catch (std::exception& e)
        {
            FILE_LOG(logERROR) << __FUNCTION_NAME__ << "- Error with parameter " << paramName << " : " << e.what();
            std::cerr << __FUNCTION_NAME__ << "- Error with parameter " << paramName << " : " << e.what() << std::endl;
        }
        return defaultVal;
    }

    template<typename T>
    T GetOptionalAttributeParameter(std::string paramName, std::string attributeName, const boost::property_tree::ptree& pt, const T& defaultVal)
    {
        try
        {
            boost::optional< const boost::property_tree::ptree& > child = pt.get_child_optional(paramName+".<xmlattr>."+attributeName);
            if( !child )
            {
                FILE_LOG(logWARNING) << __FUNCTION_NAME__ << "Value of parameter " << paramName << " not given, using default of " << defaultVal;
                return defaultVal;
            }

            return pt.get<T>(paramName+".<xmlattr>."+attributeName);
        }
        catch (std::exception& e)
        {
            FILE_LOG(logERROR) << __FUNCTION_NAME__ << "- Error with parameter " << paramName << " : " << e.what();
            std::cerr << __FUNCTION_NAME__ << "- Error with parameter " << paramName << " : " << e.what() << std::endl;
        }
        return defaultVal;
    }

    bool GetOptionalBoolParameter(std::string paramName, const boost::property_tree::ptree& pt, bool defaultBool);

    template <typename T> int Sign(T val) 
    {
        return (T(0) < val) - (val < T(0));
    }

    //inline boost::posix_time::ptime GetFutureCloseTime(boost::posix_time::ptime closeTime, std::size_t minutesDuration)
    //{
    //    return (closeTime + boost::posix_time::time_duration(0, static_cast<boost::posix_time::time_duration::min_type>(minutesDuration), 0));
    //}

    template <typename T>
    inline T Round(double d)
    {
        return static_cast<T>(floor(d + 0.5));
    }
}