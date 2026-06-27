#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

struct Config {
    int numCpu = 4;
    std::string scheduler = "rr";
    int quantumCycles = 5;
    int batchProcessFreq = 1;
    int minIns = 1000;
    int maxIns = 2000;
    int delayPerExec = 0;
    
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
        }
        
        file.close();

        if (scheduler != "fcfs" && scheduler != "rr") {
            std::cerr << "Warning: unknown scheduler \"" << scheduler
                       << "\" in config.txt (expected \"fcfs\" or \"rr\"). "
                       << "Defaulting to \"fcfs\".\n";
            scheduler = "fcfs";
        }
        
        return true;
    }
};