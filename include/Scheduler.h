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
#include "MemoryManager.h"

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

    std::mutex& getProcessMutex() { return processMutex; }
    
    MemoryManager* getMemoryManager() { return memoryManager.get(); }
    int getMemoryProcessCount() const;
    int getExternalFragmentation() const;
    std::string getMemoryMap() const;
    int getMemoryWaitQueueSize() const;
    
    const Config& getConfig() const { return config; }
    int getMemoryUsed() const;
    int getIdleTicks() const { return idleTicks; }
    int getActiveTicks() const { return activeTicks; }
    int getTotalTicks() const { return totalTicks; }

private:
    void schedulerThread();
    void workerThread(int coreId);
    void executeInstruction(std::shared_ptr<OSProcess> process, int coreId);
    void generationThread();
    void generateMemoryReport(); 
    std::string getCurrentTimeString() const;
    
    Config config;
    int numCores;
    std::queue<std::shared_ptr<OSProcess>> readyQueue;
    std::queue<std::shared_ptr<OSProcess>> memoryWaitQueue;
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

    std::unique_ptr<MemoryManager> memoryManager;
    
    std::atomic<int> totalMemoryAllocations;
    std::atomic<int> totalMemoryDeallocations;
    std::atomic<int> memoryFullCount;
    int quantumCounter;
    std::atomic<int> memoryStampCounter;
    std::chrono::steady_clock::time_point lastReportTime;
    
    std::atomic<int> idleTicks;
    std::atomic<int> activeTicks;
    std::atomic<int> totalTicks;
    
    std::string backingStoreFile = "csopesy-backing-store.txt";
};