#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <climits>

struct Config {
    int numCpu = 4;
    std::string scheduler = "rr";
    int quantumCycles = 5;
    int batchProcessFreq = 1;
    int minIns = 1000;
    int maxIns = 2000;
    int delayPerExec = 0;
    int maxOverallMem = 16384;
    int memPerFrame = 16;
    int memPerProc = 4096;
    
    bool loadFromFile(const std::string& filename = "config.txt") {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open config.txt\n";
            return false;
        }
        
        std::string key;
        while (file >> key) {
            if (key == "num-cpu") file >> numCpu;
            else if (key == "scheduler") {
                std::string schedulerType;
                if (file >> schedulerType) {
                    if (schedulerType.size() >= 2 &&
                        schedulerType.front() == '"' && schedulerType.back() == '"') {
                        schedulerType = schedulerType.substr(1, schedulerType.length() - 2);
                    }
                    std::transform(schedulerType.begin(), schedulerType.end(), schedulerType.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    scheduler = schedulerType;
                }
            }
            else if (key == "quantum-cycles") file >> quantumCycles;
            else if (key == "batch-process-freq") file >> batchProcessFreq;
            else if (key == "min-ins") file >> minIns;
            else if (key == "max-ins") file >> maxIns;
            else if (key == "delay-per-exec") file >> delayPerExec;
            else if (key == "max-overall-mem") file >> maxOverallMem;
            else if (key == "mem-per-frame") file >> memPerFrame;
            else if (key == "mem-per-proc") file >> memPerProc;
        }
        
        file.close();
        
        if (scheduler != "fcfs" && scheduler != "rr") {
            std::cerr << "Warning: unknown scheduler \"" << scheduler
                       << "\" in config.txt (expected \"fcfs\" or \"rr\"). "
                       << "Defaulting to \"fcfs\".\n";
            scheduler = "fcfs";
        }
        
        auto clamp = [](int& value, int lo, int hi, const char* name) {
            if (value < lo || value > hi) {
                std::cerr << "Warning: \"" << name << "\" value " << value
                           << " is out of range [" << lo << ", " << hi
                           << "]. Clamping.\n";
                if (value < lo) value = lo;
                if (value > hi) value = hi;
            }
        };
        
        clamp(numCpu, 1, 128, "num-cpu");
        clamp(quantumCycles, 1, INT_MAX, "quantum-cycles");
        clamp(batchProcessFreq, 1, INT_MAX, "batch-process-freq");
        clamp(minIns, 1, INT_MAX, "min-ins");
        clamp(maxIns, 1, INT_MAX, "max-ins");
        clamp(delayPerExec, 0, INT_MAX, "delay-per-exec");
        clamp(maxOverallMem, 1, INT_MAX, "max-overall-mem");
        clamp(memPerFrame, 1, INT_MAX, "mem-per-frame");
        clamp(memPerProc, 1, INT_MAX, "mem-per-proc");
        
        if (minIns > maxIns) {
            std::cerr << "Warning: min-ins (" << minIns << ") > max-ins (" << maxIns
                       << "). Swapping them.\n";
            std::swap(minIns, maxIns);
        }
        
        if (memPerProc % memPerFrame != 0) {
            std::cerr << "Warning: mem-per-proc (" << memPerProc 
                      << ") is not a multiple of mem-per-frame (" << memPerFrame 
                      << "). Adjusting mem-per-proc to " << (memPerProc / memPerFrame) * memPerFrame << "\n";
            memPerProc = (memPerProc / memPerFrame) * memPerFrame;
            if (memPerProc == 0) memPerProc = memPerFrame;
        }
        
        return true;
    }
};