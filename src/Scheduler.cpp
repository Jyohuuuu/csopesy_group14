#include "../include/Scheduler.h"
#include "../include/ConsoleSync.h"
#include "../include/PrintCommand.h"
#include "../include/FileUtils.h"
#include "../include/ProcessInstructions.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <random>

Scheduler::Scheduler(const Config& config)
    : numCores(config.numCpu), 
      running(false), 
      finishedCount(0),
      schedulerAlgorithm(config.scheduler),
      quantumCycles(config.quantumCycles),
      delayPerExec(config.delayPerExec),
      totalInstructionsExecuted(0),
      totalCPUCycles(0),
      totalCPUTime(0),
      totalIdleTime(0),
      generatingProcesses(false),
      batchProcessFreq(config.batchProcessFreq),
      minIns(config.minIns),
      maxIns(config.maxIns) {
    runningProcesses.resize(numCores, nullptr);
    processQuantumCounter.resize(numCores, 0);
}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::addProcess(std::shared_ptr<OSProcess> process) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(process);
    }
    {
        std::lock_guard<std::mutex> lock(processMutex);
        allProcesses.push_back(process);
    }
    cv.notify_one();
}

void Scheduler::start() {
    running = true;
    schedulerThread_ = std::thread(&Scheduler::schedulerThread, this);

    for (int i = 0; i < numCores; ++i) {
        workers.emplace_back(&Scheduler::workerThread, this, i);
    }
}

void Scheduler::stop() {
    running = false;
    stopGenerating();
    cv.notify_all();

    if (schedulerThread_.joinable()) schedulerThread_.join();

    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }
}

void Scheduler::startGenerating() {
    generatingProcesses = true;
    if (generatorThread.joinable()) {
        generatorThread.join();
    }
    generatorThread = std::thread(&Scheduler::generationThread, this);
}

void Scheduler::stopGenerating() {
    generatingProcesses = false;
    if (generatorThread.joinable()) {
        generatorThread.join();
    }
}

bool Scheduler::isGenerating() const {
    return generatingProcesses;
}

void Scheduler::schedulerThread() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Scheduler::generationThread() {
    static int pidCounter = 1000;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> insCountDist(100, 200);
    
    while (generatingProcesses && running) {
        std::string processName = "p" + std::to_string(pidCounter++);
        auto process = std::make_shared<OSProcess>(pidCounter, processName);
        
        std::vector<Instruction> instructions = ProcessGenerator::generateInstructions(
            minIns.load(), maxIns.load(), processName
        );
        
        for (const auto& instr : instructions) {
            process->addInstruction(instr);
        }
        
        addProcess(process);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(batchProcessFreq * 100));
    }
}

void Scheduler::workerThread(int coreId) {
    while (running) {
        std::shared_ptr<OSProcess> process = nullptr;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this] { return !readyQueue.empty() || !running; });
            if (!running) break;

            process = readyQueue.front();
            readyQueue.pop();
        }

        if (process && !process->isFinished()) {
            process->markStarted();
            process->setState(OSProcess::RUNNING);

            {
                std::lock_guard<std::mutex> lock(processMutex);
                runningProcesses[coreId] = process;
            }

            int quantumCounter = 0;
            
            while (!process->isFinished() && running) {
                process->executeNextInstruction(coreId);
                totalInstructionsExecuted++;
                quantumCounter++;
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (process->isWaiting()) {
                    process->setState(OSProcess::READY);
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        readyQueue.push(process);
                    }
                    {
                        std::lock_guard<std::mutex> lock(processMutex);
                        runningProcesses[coreId] = nullptr;
                    }
                    break;
                }
                
                if (schedulerAlgorithm == "rr") {
                    if (quantumCounter >= quantumCycles && !process->isFinished()) {
                        process->setState(OSProcess::READY);
                        {
                            std::lock_guard<std::mutex> lock(queueMutex);
                            readyQueue.push(process);
                        }
                        {
                            std::lock_guard<std::mutex> lock(processMutex);
                            runningProcesses[coreId] = nullptr;
                        }
                        break;
                    }
                } else if (schedulerAlgorithm == "fcfs") {
                }
                
                if (delayPerExec > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
                }
            }

            if (process->isFinished()) {
                process->markEnded();
                {
                    std::lock_guard<std::mutex> lock(processMutex);
                    finishedCount++;
                    runningProcesses[coreId] = nullptr;
                }
            }
        }
    }
}

bool Scheduler::allProcessesFinished() const {
    std::lock_guard<std::mutex> lock(processMutex);
    return !allProcesses.empty() &&
           finishedCount >= static_cast<int>(allProcesses.size());
}

std::shared_ptr<OSProcess> Scheduler::findProcessByName(const std::string& name) const {
    std::lock_guard<std::mutex> lock(processMutex);
    for (const auto& p : allProcesses) {
        if (p && p->getName() == name) {
            return p;
        }
    }
    return nullptr;
}

bool Scheduler::isProcessFinished(const std::string& name) const {
    auto process = findProcessByName(name);
    return process && process->isFinished();
}

float Scheduler::getCPUUtilization() const {
    std::lock_guard<std::mutex> lock(processMutex);
    int busyCores = 0;
    for (int i = 0; i < numCores; ++i) {
        if (runningProcesses[i]) busyCores++;
    }
    return (static_cast<float>(busyCores) / numCores) * 100.0f;
}

int Scheduler::getTotalProcesses() const {
    std::lock_guard<std::mutex> lock(processMutex);
    return allProcesses.size();
}

int Scheduler::getFinishedProcesses() const {
    return finishedCount;
}

int Scheduler::getTotalInstructionsExecuted() const {
    return totalInstructionsExecuted;
}

int Scheduler::getTotalCPUCycles() const {
    return totalCPUCycles;
}

void Scheduler::printStatus() const {
    std::lock_guard<std::mutex> pLock(processMutex);
    std::lock_guard<std::mutex> qLock(queueMutex);
    std::lock_guard<std::mutex> outLock(g_outputMutex);

    int busyCores = 0;
    for (int i = 0; i < numCores; ++i) {
        if (runningProcesses[i]) busyCores++;
    }
    float cpuUtil = (static_cast<float>(busyCores) / numCores) * 100.0f;

    std::cout << "\n----------------------------------------\n";
    std::cout << "CPU Utilization: " << cpuUtil << "%\n";
    std::cout << "Cores Used: " << busyCores << " | Cores Available: " << (numCores - busyCores) << "\n\n";
    
    std::cout << "Running processes:\n";
    bool anyRunning = false;
    for (int i = 0; i < numCores; ++i) {
        if (runningProcesses[i]) {
            auto p = runningProcesses[i];
            std::cout << "- " << p->getName() 
                      << " (Started: " << p->getStartTimeString() << ")  "
                      << "Core: " << i << "  "
                      << p->getCommandCounter() 
                      << " / " << p->getTotalCommands() << "\n";
            anyRunning = true;
        }
    }
    if (!anyRunning) std::cout << "(none)\n";

    std::cout << "\nFinished processes:\n";
    bool anyFinished = false;
    for (const auto& p : allProcesses) {
        if (p && p->isFinished()) {
            std::cout << "- " << p->getName() 
                      << " (Started: " << p->getStartTimeString() << ")  "
                      << "Finished  "
                      << p->getTotalCommands() 
                      << " / " << p->getTotalCommands() << "\n";
            anyFinished = true;
        }
    }
    if (!anyFinished) std::cout << "(none)\n";

    std::cout << "\nReady queue size: " << readyQueue.size() << "\n";
    std::cout << "Total processes: " << allProcesses.size()
              << " | Finished: " << finishedCount << "\n";
    std::cout << "Instructions executed: " << totalInstructionsExecuted << "\n";
    std::cout << "----------------------------------------\n";
}

void Scheduler::printUtilizationReport() const {
    printStatus();
}

void Scheduler::saveUtilizationReport(const std::string&) const {
    std::lock_guard<std::mutex> pLock(processMutex);
    std::lock_guard<std::mutex> qLock(queueMutex);
    
    std::string reportPath = FileUtils::getReportPath();
    
    std::ofstream file(reportPath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << reportPath << " for writing\n";
        return;
    }

    file << "========================================\n";
    file << "  CPU UTILIZATION REPORT\n";
    file << "  Generated: " << getCurrentTimeString() << "\n";
    file << "========================================\n";

    int busyCores = 0;
    for (int i = 0; i < numCores; ++i) {
        if (runningProcesses[i]) busyCores++;
    }
    float cpuUtil = (static_cast<float>(busyCores) / numCores) * 100.0f;

    file << "CPU Utilization: " << cpuUtil << "%\n";
    file << "Cores Used: " << busyCores << " | Cores Available: " << (numCores - busyCores) << "\n\n";
    
    file << "Running processes:\n";
    bool anyRunning = false;
    for (int i = 0; i < numCores; ++i) {
        if (runningProcesses[i]) {
            auto p = runningProcesses[i];
            file << "- " << p->getName() 
                 << " (Started: " << p->getStartTimeString() << ")  "
                 << "Core: " << i << "  "
                 << p->getCommandCounter() 
                 << " / " << p->getTotalCommands() << "\n";
            anyRunning = true;
        }
    }
    if (!anyRunning) file << "(none)\n";

    file << "\nFinished processes:\n";
    bool anyFinished = false;
    for (const auto& p : allProcesses) {
        if (p && p->isFinished()) {
            file << "- " << p->getName() 
                 << " (Started: " << p->getStartTimeString() << ")  "
                 << "Finished  "
                 << p->getTotalCommands() 
                 << " / " << p->getTotalCommands() << "\n";
            anyFinished = true;
        }
    }
    if (!anyFinished) file << "(none)\n";

    file << "\nReady queue size: " << readyQueue.size() << "\n";
    file << "Total processes: " << allProcesses.size()
         << " | Finished: " << finishedCount << "\n";
    file << "Instructions executed: " << totalInstructionsExecuted << "\n";
    file << "========================================\n";
    
    file.close();
    std::cout << "Report saved to: " << reportPath << "\n";
}

void Scheduler::executeInstruction(std::shared_ptr<OSProcess>, int) {
    // Placeholder - actual execution happens in workerThread
}

std::string Scheduler::getCurrentTimeString() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}