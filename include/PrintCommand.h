#pragma once
#include "ICommand.h"
#include "FileUtils.h"
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
    static std::mutex fileMutex;

public:
    PrintCommand(const std::string& msg, bool enableOutput = true) 
        : message(msg), enableFileOutput(enableOutput) {}

    void execute(int coreId, int pid, const std::string& processName) override {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        std::string timestamp = ss.str();

        if (enableFileOutput) {
            std::string filename = FileUtils::getProcessLogPath(pid, processName);
            std::lock_guard<std::mutex> lock(fileMutex);
            std::ofstream file(filename, std::ios::app);
            if (file.is_open()) {
                file << "[" << timestamp << "] Core " << coreId 
                     << " | " << message << std::endl;
                file.close();
            }
        }
    }

    void setFileOutput(bool enabled) {
        enableFileOutput = enabled;
    }
};