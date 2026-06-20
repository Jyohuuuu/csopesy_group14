#pragma once
#include "ICommand.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <ctime>

class PrintCommand : public ICommand {
private:
    std::string message;
    bool enableFileOutput;

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

        // Output to console
        std::cout << "[" << timestamp << "] Core " << coreId 
                  << " | Process " << processName << " (PID: " << pid 
                  << "): " << message << std::endl;

        // Output to file if enabled
        if (enableFileOutput) {
            std::string filename = "process_" + std::to_string(pid) + "_" + processName + ".txt";
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