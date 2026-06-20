#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include "Process.h"

class Scheduler {
public:
    Scheduler(int numCores);
    ~Scheduler();

    void addProcess(std::shared_ptr<Process> process);
    void start();
    void stop();
    bool allProcessesFinished() const;
    void printStatus() const;

private:
    void schedulerThread();
    void workerThread(int coreId);

    int numCores;
    std::queue<std::shared_ptr<Process>> readyQueue;
    std::vector<std::shared_ptr<Process>> allProcesses;
    std::vector<std::thread> workers;
    std::thread scheduler;

    mutable std::mutex queueMutex;
    mutable std::mutex processMutex;
    std::condition_variable cv;
    std::atomic<bool> running;
    std::atomic<int> finishedCount;
    std::vector<std::shared_ptr<Process>> runningProcesses;
};