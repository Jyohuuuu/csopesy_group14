#pragma once
#include <string>
#include <sys/stat.h>
#include <direct.h>
#include <iostream>

class FileUtils {
public:
    static bool createDirectory(const std::string& path) {
        #ifdef _WIN32
            return _mkdir(path.c_str()) == 0 || errno == EEXIST;
        #else
            return mkdir(path.c_str(), 0777) == 0 || errno == EEXIST;
        #endif
    }
    
    static std::string getProcessLogPath(int pid, const std::string& processName) {
        std::string folder = "process_logs";
        createDirectory(folder);
        return folder + "/process_" + std::to_string(pid) + "_" + processName + ".txt";
    }
    
    static std::string getReportPath() {
        std::string folder = "reports";
        createDirectory(folder);
        return folder + "/csopesy-log.txt";
    }
};