#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <climits>
#include <random>

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
    int minMemPerProc = 256;
    int maxMemPerProc = 4096;
    
    bool loadFromFile(const std::string& filename = "config.txt") {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open config.txt\n";
            return false;
        }
        
        std::string key;
        while (file >> key) {
            if (key == "num-cpu") {
                file >> numCpu;
            }
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
            else if (key == "quantum-cycles") {
                file >> quantumCycles;
            }
            else if (key == "batch-process-freq") {
                file >> batchProcessFreq;
            }
            else if (key == "min-ins") {
                file >> minIns;
            }
            else if (key == "max-ins") {
                file >> maxIns;
            }
            else if (key == "delay-per-exec") {
                file >> delayPerExec;
            }
            else if (key == "max-overall-mem") {
                file >> maxOverallMem;
            }
            else if (key == "mem-per-frame") {
                file >> memPerFrame;
            }
            else if (key == "min-mem-per-proc") {
                file >> minMemPerProc;
            }
            else if (key == "max-mem-per-proc") {
                file >> maxMemPerProc;
            }
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
        
        if (scheduler == "rr") {
            clamp(quantumCycles, 1, INT_MAX, "quantum-cycles");
        } else {
            if (quantumCycles < 0) {
                quantumCycles = 0;
            }
        }
        
        clamp(batchProcessFreq, 1, INT_MAX, "batch-process-freq");
        clamp(minIns, 1, INT_MAX, "min-ins");
        clamp(maxIns, 1, INT_MAX, "max-ins");
        clamp(delayPerExec, 0, INT_MAX, "delay-per-exec");
        clamp(maxOverallMem, 1, INT_MAX, "max-overall-mem");
        clamp(memPerFrame, 1, INT_MAX, "mem-per-frame");
        
        auto validatePowerOfTwo = [](int& value, const char* name) {
            if (value < 64 || value > 32768) {
                std::cerr << "Warning: \"" << name << "\" value " << value
                           << " is outside range [64, 32768]. Clamping.\n";
                if (value < 64) value = 64;
                if (value > 32768) value = 32768;
            }
            if ((value & (value - 1)) != 0) {
                int original = value;
                int power = 1;
                while (power * 2 <= value) {
                    power *= 2;
                }
                value = power;
                std::cerr << "Warning: \"" << name << "\" value " << original
                           << " is not a power of 2. Rounding down to " << value << "\n";
            }
        };
        
        validatePowerOfTwo(minMemPerProc, "min-mem-per-proc");
        validatePowerOfTwo(maxMemPerProc, "max-mem-per-proc");
        
        if (minMemPerProc > maxMemPerProc) {
            std::cerr << "Warning: min-mem-per-proc (" << minMemPerProc 
                       << ") > max-mem-per-proc (" << maxMemPerProc
                       << "). Swapping them.\n";
            std::swap(minMemPerProc, maxMemPerProc);
        }
        
        if (minIns > maxIns) {
            std::cerr << "Warning: min-ins (" << minIns << ") > max-ins (" << maxIns
                       << "). Swapping them.\n";
            std::swap(minIns, maxIns);
        }
        
        if (maxOverallMem % memPerFrame != 0) {
            std::cerr << "Warning: max-overall-mem (" << maxOverallMem 
                      << ") is not a multiple of mem-per-frame (" << memPerFrame 
                      << "). Adjusting max-overall-mem to " 
                      << (maxOverallMem / memPerFrame) * memPerFrame << "\n";
            maxOverallMem = (maxOverallMem / memPerFrame) * memPerFrame;
            if (maxOverallMem == 0) maxOverallMem = memPerFrame;
        }
        
        auto validateMultiple = [&](int& value, const char* name) {
            if (value % memPerFrame != 0) {
                std::cerr << "Warning: " << name << " (" << value 
                          << ") is not a multiple of mem-per-frame (" << memPerFrame 
                          << "). Adjusting to " << (value / memPerFrame) * memPerFrame << "\n";
                value = (value / memPerFrame) * memPerFrame;
                if (value < memPerFrame) value = memPerFrame;
            }
        };
        
        validateMultiple(minMemPerProc, "min-mem-per-proc");
        validateMultiple(maxMemPerProc, "max-mem-per-proc");
        
        return true;
    }
    
    int getRandomMemorySize() const {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        std::vector<int> validSizes;
        int size = 64;
        while (size <= 32768) {
            if (size >= minMemPerProc && size <= maxMemPerProc) {
                validSizes.push_back(size);
            }
            size *= 2;
        }
        
        if (validSizes.empty()) {
            return minMemPerProc;
        }
        
        std::uniform_int_distribution<> dist(0, validSizes.size() - 1);
        return validSizes[dist(gen)];
    }
};