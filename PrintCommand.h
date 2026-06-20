#pragma once
#include "ICommand.h"
#include "ConsoleSync.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <ctime>
#include <mutex>

class PrintCommand : public ICommand {
private:
    std::string message;
    bool enableFileOutput;

    // serialize file writes to avoid interleaving
    static std::mutex fileMutex;

public:
    PrintCommand(const std::string& msg, bool enableOutput = true)
        : message(msg), enableFileOutput(enableOutput) {}

    void execute(int coreId, int pid, const std::string& processName) override {
        auto now = std::chrono::system_clock::now();
        std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);

        std::tm local_tm{};
    #ifdef _WIN32
        localtime_s(&local_tm, &time_t_now);
    #else
        localtime_r(&time_t_now, &local_tm);
    #endif

        std::ostringstream ts;
        ts << "(" << std::put_time(&local_tm, "%m/%d/%Y %I:%M:%S%p") << ")";

        std::string line = ts.str() + " Core:" + std::to_string(coreId) +
                           " \"" + message + "\"";

        // Do NOT print to console here — background output is redirected to files only.

        if (enableFileOutput) {
            std::string filename = "process_" + std::to_string(pid) + "_" + processName + ".txt";
            std::lock_guard<std::mutex> lock(fileMutex);
            std::ofstream file(filename, std::ios::app);
            if (file.is_open()) {
                file << line << '\n';
            }
        }
    }

    void setFileOutput(bool enabled) {
        enableFileOutput = enabled;
    }
};

std::mutex PrintCommand::fileMutex;