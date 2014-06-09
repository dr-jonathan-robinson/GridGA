#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>

#include <sstream>
#include <string>
#include <stdio.h>

inline std::string NowTime();

enum TLogLevel {logERROR, logWARNING, logINFO, logINFONOTIMESTAMP, logINFOwithCOUT, logDEBUG, logSPAM, logDEBUG2, logDEBUG3, logDEBUG4};

template <typename T>
class Log
{
public:
    Log();
    virtual ~Log();
    std::ostringstream& Get(TLogLevel level = logINFO);
public:
    static TLogLevel& ReportingLevel();
    static std::string ToString(TLogLevel level);
protected:
    std::ostringstream os;
private:
    Log(const Log&);
    Log& operator =(const Log&);
    TLogLevel mLevel;
};

template <typename T>
Log<T>::Log()
{
}

template <typename T>
std::ostringstream& Log<T>::Get(TLogLevel level)
{
    mLevel = level;
	if (level != logINFONOTIMESTAMP)
	{
		os << NowTime();
		os << " " << ToString(level) << ": ";
	}

    return os;
}

template <typename T>
Log<T>::~Log()
{
    os << std::endl;
    T::Output(os.str(), mLevel);
}

template <typename T>
TLogLevel& Log<T>::ReportingLevel()
{
    static TLogLevel reportingLevel = logDEBUG4;
    return reportingLevel;
}

template <typename T>
std::string Log<T>::ToString(TLogLevel level)
{
	static const char* const buffer[] = {">>ERROR<<", "Warning", "info", "InfoNoTimeStamp", "Info", "debug", "spam", "debug2", "debug3", "debug4"};
    return buffer[level];
}

class Output2FILE
{
public:
    static FILE*& Stream();
    static void Output(const std::string& msg, TLogLevel level);
};

inline FILE*& Output2FILE::Stream()
{
    static FILE* pStream = stderr;
    return pStream;
}

inline void Output2FILE::Output(const std::string& msg, TLogLevel level)
{   
    FILE* pStream = Stream();
    if (!pStream)
        return;
    fprintf(pStream, "%s", msg.c_str());
    fflush(pStream);
    if (level == logINFOwithCOUT)
    {
        std::cout << msg << std::endl;
    }
}

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#   if defined (BUILDING_FILELOG_DLL)
#       define FILELOG_DECLSPEC   __declspec (dllexport)
#   elif defined (USING_FILELOG_DLL)
#       define FILELOG_DECLSPEC   __declspec (dllimport)
#   else
#       define FILELOG_DECLSPEC
#   endif // BUILDING_DBSIMPLE_DLL
#else
#   define FILELOG_DECLSPEC
#endif // _WIN32

class FILELOG_DECLSPEC FILELog : public Log<Output2FILE> {};

#ifndef FILELOG_MAX_LEVEL
#define FILELOG_MAX_LEVEL logSPAM
#endif

#define FILE_LOG(level) \
    if (level > FILELOG_MAX_LEVEL) ;\
    else if (level > FILELog::ReportingLevel() || !Output2FILE::Stream()) ; \
    else FILELog().Get(level)



inline std::string NowTime()
{
	return boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time()); // universal_time()); // microsec_clock::universal_time()
}


class Logger
{
public:
    static void Initialise();
    static void Initialise(std::string logFileName);
    static void Close();
private:
    static bool mInitialised;
};

inline void Logger::Initialise()
{
    if (mInitialised)
    {
        return;
    }

    std::ostringstream sf;
    sf << "DeepThought.log";

    FILE* pFile = fopen(sf.str().c_str(), "w");
    Output2FILE::Stream() = pFile;
    mInitialised = true;
    std::ostringstream s;

    s << "DeepThought built on " << __DATE__ << " at " << __TIME__ << std::endl;

    fprintf(pFile, "%s", s.str().c_str());
    fflush(pFile);
}

inline void Logger::Initialise(std::string logFileName)
{
    if (mInitialised)
    {
        return;
    }

    FILE* pFile = fopen(logFileName.c_str(), "w");
    Output2FILE::Stream() = pFile;
    mInitialised = true;
    std::ostringstream s;

    s << "DeepThought built on " << __DATE__ << " at " << __TIME__ << std::endl;
    fprintf(pFile, "%s", s.str().c_str());
    fflush(pFile);
}


inline void Logger::Close(void)
{
    FILE* pStream = Output2FILE::Stream();
    fclose(pStream);
    mInitialised = false;
}