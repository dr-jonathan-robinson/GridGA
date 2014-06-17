#pragma once
#include "stdafx.hpp"

namespace DeepThoughtLib
{
    namespace FileUtils
    {

        inline std::string GetExePath() //char* argv0)
        {
#ifdef _WIN32
#include <Windows.h>
            char buffer[MAX_PATH];//always use MAX_PATH for filepaths
            GetModuleFileName(NULL,buffer,sizeof(buffer));
            std::ostringstream s;
            s << buffer;
            
            boost::filesystem::path fullPath(s.str());
            //boost::filesystem::path full_path( boost::filesystem::initial_path<boost::filesystem::path>() );

            //full_path = boost::filesystem::system_complete( boost::filesystem::path( argv0 ) );

            //std::cout << full_path << std::endl;
            //std::cout << full_path.parent_path() << std::endl;

            //Without file name
            return  fullPath.parent_path().string();
#elif __APPLE__
//#include <mach-o/dyld.h>
//            char path[1024];
//            uint32_t size = sizeof(path);
//            if (_NSGetExecutablePath(path, &size) == 0)
//                printf("executable path is %s\n", path);
//            else
//                printf("buffer too small; need size %u\n", size);
return "TODO";
#elif __linux
#include <limits.h>
            char exepath[PATH_MAX] = {0};
            readlink("/proc/self/exe", exepath, sizeof(exepath));
            std::ostringstream s;
            s << exepath;
            boost::filesystem::path fullPath(s.str());
            return fullPath.parent_path().string();
#elif __unix // all unices not caught above
            // Unix
#elif __posix
            // POSIX
#endif
        }

        //______________________________________________________________________________________________________________

        inline std::string GetHomeDir(void)
        {
#ifdef _WIN32
            std::string s = getenv("USERPROFILE");
            if (boost::filesystem::exists(s))
            {
                return s;
            }
            std::ostringstream s2;
            s2 << getenv("HOMEDRIVE") << getenv("HOMEDIR");
            return s2.str();
            //#include <windows.h>
            //#include <shlobj.h>
            //TCHAR path[MAX_PATH];
            //if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL|CSIDL_FLAG_CREATE, NULL, 0, path)))
            //{
            //    // need to convert to OEM codepage so that fstream can use
            //    // it properly on international systems.
            //    TCHAR oemPath[MAX_PATH];
            //    CharToOem(path, oemPath);
            //    std::string s(oemPath);
            //    return s;
            //    //mHomePath = oemPath;
            //}
            //else
            //{
            //    return "./";
            //}
#else
            std::ostringstream s;
            struct passwd* pwd = getpwuid(getuid());
            if (pwd)
            {
                s << pwd->pw_dir;
            }
            else
            {
                // try the $HOME environment variable
                s << getenv("HOME");
            }

            return s.str();
#endif
        }

        //______________________________________________________________________________________________________________

    }
}
