#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "OSProcess.h"
#include "Config.h"

class Scheduler {
public:
    Scheduler(const Config& config);
    ~Scheduler();

    void addProcess(std::shared_ptr<OSProcess> process);
    void start();
    void stop();
    bool allProcessesFinished() const;
    void printStatus() const;
    void printUtilizationReport() const;
    void saveUtilizationReport(const std::string& filename = "csopesy-log.txt") const;
    
    std::shared_ptr<OSProcess> findProcessByName(const std::string& name) const;
    bool isProcessFinished(const std::string& name) const;
    
    float getCPUUtilization() const;
    int getTotalProcesses() const;
    int getFinishedProcesses() const;
    int getTotalInstructionsExecuted() const;
    int getTotalCPUCycles() const;
    
    void startGenerating();
    void stopGenerating();
    bool isGenerating() const;

    int getMinIns() const { return minIns.load(); }
    int getMaxIns() const { return maxIns.load(); }
    
private:
    void schedulerThread();
    void workerThread(int coreId);
    void executeInstruction(std::shared_ptr<OSProcess> process, int coreId);
    void generationThread();
    
    std::string getCurrentTimeString() const;
    
    int numCores;
    std::queue<std::shared_ptr<OSProcess>> readyQueue;
    std::vector<std::shared_ptr<OSProcess>> allProcesses;
    std::vector<std::thread> workers;
    std::thread schedulerThread_;
    
    mutable std::mutex queueMutex;
    mutable std::mutex processMutex;
    std::condition_variable cv;
    std::atomic<bool> running;
    std::atomic<int> finishedCount;
    std::vector<std::shared_ptr<OSProcess>> runningProcesses;
    std::vector<int> processQuantumCounter;
    
    std::string schedulerAlgorithm;
    int quantumCycles;
    int delayPerExec;
    
    mutable std::atomic<int> totalInstructionsExecuted;
    mutable std::atomic<int> totalCPUCycles;
    mutable std::atomic<float> totalCPUTime;
    mutable std::atomic<float> totalIdleTime;
    
    std::atomic<bool> generatingProcesses;
    std::atomic<int> batchProcessFreq;
    std::atomic<int> minIns;
    std::atomic<int> maxIns;
    std::thread generatorThread;
};