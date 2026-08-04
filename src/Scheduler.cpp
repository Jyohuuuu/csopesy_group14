#include "../include/Scheduler.h"
#include "../include/ConsoleSync.h"
#include "../include/PrintCommand.h"
#include "../include/FileUtils.h"
#include "../include/ProcessInstructions.h"
#include "../include/MemoryManager.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <random>
#include <iomanip>
#include <filesystem>

Scheduler::Scheduler(const Config& config)
    : config(config),
      numCores(config.numCpu), 
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
      maxIns(config.maxIns),
      memoryManager(std::make_unique<MemoryManager>(
          config.maxOverallMem,
          config.memPerFrame,
          config.minMemPerProc
      )),
      totalMemoryAllocations(0),
      totalMemoryDeallocations(0),
      memoryFullCount(0),
      quantumCounter(0),
      memoryStampCounter(0),
      idleTicks(0),
      activeTicks(0),
      totalTicks(0) {
    runningProcesses.resize(numCores, nullptr);
    processQuantumCounter.resize(numCores, 0);
    lastReportTime = std::chrono::steady_clock::now();
}
Scheduler::~Scheduler() {
    stop();
}

void Scheduler::addProcess(std::shared_ptr<OSProcess> process) {
    if (process) {
        process->setMemoryManager(std::shared_ptr<MemoryManager>(memoryManager.get(), [](MemoryManager*){}));
    }
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        memoryWaitQueue.push(process);
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
    
    if (memoryManager) {
        memoryManager->saveToBackingStore(backingStoreFile);
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
        std::shared_ptr<OSProcess> candidate = nullptr;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!memoryWaitQueue.empty()) {
                candidate = memoryWaitQueue.front();
            }
        }

        if (candidate) {
            bool allocated = false;
            {
                std::lock_guard<std::mutex> lock(processMutex);
                allocated = memoryManager->allocateMemory(candidate);
            }

            if (allocated) {
                candidate->setHasMemory(true);
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    memoryWaitQueue.pop();
                    readyQueue.push(candidate);
                }
                cv.notify_all();
                continue;
            } else {
                memoryFullCount++;
                memoryManager->saveToBackingStore(backingStoreFile);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Scheduler::generationThread() {
    static int pidCounter = 1000;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> insCountDist(100, 200);
    std::uniform_int_distribution<> memSizeDist(config.minMemPerProc, config.maxMemPerProc);

    while (generatingProcesses && running) {
        std::string processName = "p" + std::to_string(pidCounter++);
        auto process = std::make_shared<OSProcess>(pidCounter, processName);

        int procMemSize = memSizeDist(gen);
        process->setMemorySize(procMemSize);

        std::vector<Instruction> instructions = ProcessGenerator::generateInstructions(
            minIns.load(), maxIns.load(), processName, procMemSize
        );

        for (const auto& instr : instructions) {
            process->addInstruction(instr);
        }

        addProcess(process);

        std::this_thread::sleep_for(std::chrono::milliseconds(batchProcessFreq * 100));
    }
}

void Scheduler::generateMemoryReport() {
    std::lock_guard<std::mutex> lock(processMutex);

    int qq = ++memoryStampCounter;

    const std::string outputDir = "memory_stamps";
    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    if (ec) {
        std::cerr << "Error: Could not create directory " << outputDir
                   << ": " << ec.message() << "\n";
        return;
    }

    std::string filename = outputDir + "/memory_stamp_" + std::to_string(qq) + ".txt";
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << " for writing\n";
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tmNow = *std::localtime(&time_t_now);
    std::stringstream ts;
    ts << std::put_time(&tmNow, "%m/%d/%Y %I:%M:%S");
    ts << (tmNow.tm_hour >= 12 ? "PM" : "AM");

    file << "Timestamp: (" << ts.str() << ")\n";
    file << "Number of processes in memory: " << memoryManager->getProcessCount() << "\n";
    file << "Total external fragmentation in KB: " << memoryManager->calculateExternalFragmentation() / 1024 << "\n";
    file << "\n";
    file << memoryManager->getASCIIMemoryMap();

    file.close();
}

void Scheduler::workerThread(int coreId) {
    while (running) {
        std::shared_ptr<OSProcess> process = nullptr;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (readyQueue.empty()) {
                cv.wait_for(lock, std::chrono::milliseconds(100), [this] { 
                    return !readyQueue.empty() || !running; 
                });
                if (!running) break;
                if (readyQueue.empty()) {
                    idleTicks++;
                    totalTicks++;
                    continue;
                }
            }

            process = readyQueue.front();
            readyQueue.pop();
        }

        if (!process || process->isFinished()) {
            idleTicks++;
            totalTicks++;
            continue;
        }
        
        if (process->hasMemoryViolation()) {
            process->setState(OSProcess::FINISHED);
            process->markEnded();
            {
                std::lock_guard<std::mutex> lock(processMutex);
                memoryManager->releaseMemory(process);
                process->setHasMemory(false);
                totalMemoryDeallocations++;
                finishedCount++;
                runningProcesses[coreId] = nullptr;
            }
            continue;
        }
        
        process->markStarted();
        process->setState(OSProcess::RUNNING);

        {
            std::lock_guard<std::mutex> lock(processMutex);
            runningProcesses[coreId] = process;
        }

        int quantumCounterLocal = 0;
        activeTicks++;
        totalTicks++;
        
        while (!process->isFinished() && running) {
            try {
                if (process->hasMemoryViolation()) {
                    process->setState(OSProcess::FINISHED);
                    process->markEnded();
                    {
                        std::lock_guard<std::mutex> lock(processMutex);
                        memoryManager->releaseMemory(process);
                        process->setHasMemory(false);
                        totalMemoryDeallocations++;
                        finishedCount++;
                        runningProcesses[coreId] = nullptr;
                    }
                    break;
                }
                
                process->executeNextInstruction(coreId);
                totalInstructionsExecuted++;
                quantumCounterLocal++;
                quantumCounter++;
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                if (quantumCounter >= quantumCycles) {
                    quantumCounter = 0;
                    generateMemoryReport();
                }
                
                if (process->isWaiting()) {
                    process->setState(OSProcess::READY);
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        readyQueue.push(process);
                        cv.notify_one();
                    }
                    {
                        std::lock_guard<std::mutex> lock(processMutex);
                        runningProcesses[coreId] = nullptr;
                    }
                    break;
                }
                
                if (schedulerAlgorithm == "rr" && quantumCounterLocal >= quantumCycles && !process->isFinished()) {
                    process->setState(OSProcess::READY);
                {
                    std::lock_guard<std::mutex> lock(processMutex);
                    memoryManager->releaseMemory(process);
                    process->setHasMemory(false);
                }
    
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    memoryWaitQueue.push(process);
                    cv.notify_one();
                }
                {
                    std::lock_guard<std::mutex> lock(processMutex);
                    runningProcesses[coreId] = nullptr;
                }
    break;
}
                
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Exception in worker " << coreId << ": " << e.what() << "\n";
                break;
            }
        }

        if (process->isFinished()) {
            process->markEnded();
            {
                std::lock_guard<std::mutex> lock(processMutex);
                memoryManager->releaseMemory(process);
                process->setHasMemory(false);
                totalMemoryDeallocations++;
                finishedCount++;
                runningProcesses[coreId] = nullptr;
            }
        }
    }
}

int Scheduler::getMemoryUsed() const {
    if (memoryManager) {
        return memoryManager->getUsedMemory();
    }
    return 0;
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

int Scheduler::getMemoryProcessCount() const {
    std::lock_guard<std::mutex> lock(processMutex);
    return memoryManager->getProcessCount();
}

int Scheduler::getExternalFragmentation() const {
    std::lock_guard<std::mutex> lock(processMutex);
    return memoryManager->calculateExternalFragmentation();
}

std::string Scheduler::getMemoryMap() const {
    std::lock_guard<std::mutex> lock(processMutex);
    return memoryManager->getMemoryMap();
}

int Scheduler::getMemoryWaitQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return static_cast<int>(memoryWaitQueue.size());
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
    
    std::cout << "Memory Information:\n";
    std::cout << "  Processes in memory: " << memoryManager->getProcessCount() << "\n";
    std::cout << "  External fragmentation: " << memoryManager->calculateExternalFragmentation() / 1024 << " KB\n";
    std::cout << "  Memory full events: " << memoryFullCount << "\n";
    std::cout << "  Pages paged in: " << memoryManager->getNumPagesPagedIn() << "\n";
    std::cout << "  Pages paged out: " << memoryManager->getNumPagesPagedOut() << "\n\n";
    
    std::cout << "Running processes:\n";
    bool anyRunning = false;
    for (int i = 0; i < numCores; ++i) {
        if (runningProcesses[i]) {
            auto p = runningProcesses[i];
            std::cout << "- " << p->getName() 
                      << " (Memory: " << p->getMemorySize() << " bytes)  "
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
    std::cout << "Waiting for memory: " << memoryWaitQueue.size() << "\n";
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
    
    file << "Memory Information:\n";
    file << "  Processes in memory: " << memoryManager->getProcessCount() << "\n";
    file << "  External fragmentation: " << memoryManager->calculateExternalFragmentation() / 1024 << " KB\n";
    file << "  Memory full events: " << memoryFullCount << "\n";
    file << "  Pages paged in: " << memoryManager->getNumPagesPagedIn() << "\n";
    file << "  Pages paged out: " << memoryManager->getNumPagesPagedOut() << "\n\n";
    file << "Memory Map:\n";
    file << memoryManager->getMemoryMap() << "\n";
    
    file << "Running processes:\n";
    bool anyRunning = false;
    for (int i = 0; i < numCores; ++i) {
        if (runningProcesses[i]) {
            auto p = runningProcesses[i];
            file << "- " << p->getName() 
                 << " (Memory: " << p->getMemorySize() << " bytes)  "
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
    file << "Waiting for memory: " << memoryWaitQueue.size() << "\n";
    file << "Total processes: " << allProcesses.size()
         << " | Finished: " << finishedCount << "\n";
    file << "Instructions executed: " << totalInstructionsExecuted << "\n";
    file << "========================================\n";
    
    file.close();
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