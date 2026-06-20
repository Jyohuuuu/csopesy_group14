#include "Scheduler.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <thread>

Scheduler::Scheduler(int numCores) 
    : numCores(numCores), running(false), finishedCount(0) {
    runningProcesses.resize(numCores, nullptr);
}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::addProcess(std::shared_ptr<Process> process) {
    std::lock_guard<std::mutex> lock(queueMutex);
    readyQueue.push(process);
    allProcesses.push_back(process);
    cv.notify_one();
}

void Scheduler::start() {
    running = true;
    scheduler = std::thread(&Scheduler::schedulerThread, this);
    
    for (int i = 0; i < numCores; ++i) {
        workers.emplace_back(&Scheduler::workerThread, this, i);
    }
}

void Scheduler::stop() {
    running = false;
    cv.notify_all();
    
    if (scheduler.joinable()) {
        scheduler.join();
    }
    
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void Scheduler::schedulerThread() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::shared_ptr<Process> Scheduler::getNextReadyProcess() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (readyQueue.empty()) {
        return nullptr;
    }
    
    auto process = readyQueue.front();
    readyQueue.pop();
    return process;
}

void Scheduler::workerThread(int coreId) {
    while (running) {
        auto process = getNextReadyProcess();
        
        if (process && !process->isFinished()) {
            process->setState(Process::RUNNING);
            {
                std::lock_guard<std::mutex> lock(processMutex);
                if (coreId >= 0 && coreId < static_cast<int>(runningProcesses.size())) {
                    runningProcesses[coreId] = process;
                }
            }
            
            while (!process->isFinished() && running) {
                process->executeCurrentCommand(coreId);
                process->moveToNextLine();
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            if (process->isFinished()) {
                std::lock_guard<std::mutex> lock(processMutex);
                finishedCount++;
                if (coreId >= 0 && coreId < static_cast<int>(runningProcesses.size())) {
                    runningProcesses[coreId] = nullptr;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

bool Scheduler::allProcessesFinished() const {
    return finishedCount >= static_cast<int>(allProcesses.size()) && !allProcesses.empty();
}

void Scheduler::printStatus() const {
    std::lock_guard<std::mutex> lock(processMutex);
    
    std::cout << "\n=== Scheduler Status ===" << std::endl;
    std::cout << "Total processes: " << allProcesses.size() << std::endl;
    std::cout << "Finished: " << finishedCount << std::endl;
    
    std::cout << "\nRunning processes:" << std::endl;
    for (int i = 0; i < numCores; ++i) {
        if (i < static_cast<int>(runningProcesses.size()) && runningProcesses[i]) {
            auto p = runningProcesses[i];
            std::cout << "  Core " << i << ": " << p->getName() 
                      << " (PID: " << p->getPID() << ")" << std::endl;
        } else {
            std::cout << "  Core " << i << ": Idle" << std::endl;
        }
    }
    
    std::cout << "Ready queue size: " << readyQueue.size() << std::endl;
    std::cout << std::endl;
}