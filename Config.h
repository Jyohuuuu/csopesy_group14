#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

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
                file >> schedulerType;
                if (schedulerType.front() == '"' && schedulerType.back() == '"') {
                    schedulerType = schedulerType.substr(1, schedulerType.length() - 2);
                }
                scheduler = schedulerType;
            }
            else if (key == "quantum-cycles") file >> quantumCycles;
            else if (key == "batch-process-freq") file >> batchProcessFreq;
            else if (key == "min-ins") file >> minIns;
            else if (key == "max-ins") file >> maxIns;
            else if (key == "delay-per-exec") file >> delayPerExec;
        }
        
        file.close();
        return true;
    }
};