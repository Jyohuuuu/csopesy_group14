#include "Scheduler.h"
#include "ConsoleSync.h"
#include <iostream>
#include <chrono>
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

    if (scheduler.joinable()) scheduler.join();

    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }
}

void Scheduler::schedulerThread() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void Scheduler::workerThread(int coreId) {
    while (running) {
        std::shared_ptr<Process> process = nullptr;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this] { return !readyQueue.empty() || !running; });
            if (!running) break;

            process = readyQueue.front();
            readyQueue.pop();
        }

        if (process && !process->isFinished()) {
            process->setState(Process::RUNNING);

            {
                std::lock_guard<std::mutex> lock(processMutex);
                runningProcesses[coreId] = process;
            }

            while (!process->isFinished() && running) {
                process->executeCurrentCommand(coreId);
                process->moveToNextLine();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (process->isFinished()) {
                std::lock_guard<std::mutex> lock(processMutex);
                finishedCount++;
                runningProcesses[coreId] = nullptr;
            }
        }
    }
}

bool Scheduler::allProcessesFinished() const {
    std::lock_guard<std::mutex> lock(processMutex);
    return !allProcesses.empty() &&
           finishedCount >= static_cast<int>(allProcesses.size());
}

void Scheduler::printStatus() const {
    // lock scheduler data first
    std::lock_guard<std::mutex> pLock(processMutex);
    std::lock_guard<std::mutex> qLock(queueMutex);

    // then lock shared output mutex so no PrintCommand can interleave
    std::lock_guard<std::mutex> outLock(g_outputMutex);

    std::cout << "\n----------------------------------------\n";
    std::cout << "Running processes:\n";

    bool anyRunning = false;
    for (int i = 0; i < numCores; ++i) {
        if (runningProcesses[i]) {
            auto p = runningProcesses[i];
            std::cout << p->getName()
                      << "\tCore: " << i
                      << "\t" << p->getCommandCounter()
                      << " / " << p->getTotalCommands() << "\n";
            anyRunning = true;
        }
    }
    if (!anyRunning) std::cout << "(none)\n";

    std::cout << "\nFinished processes:\n";
    bool anyFinished = false;
    for (const auto& p : allProcesses) {
        if (p && p->isFinished()) {
            std::cout << p->getName()
                      << "\tFinished\t"
                      << p->getTotalCommands() << " / " << p->getTotalCommands() << "\n";
            anyFinished = true;
        }
    }
    if (!anyFinished) std::cout << "(none)\n";

    std::cout << "\nReady queue size: " << readyQueue.size() << "\n";
    std::cout << "Total processes: " << allProcesses.size()
              << " | Finished: " << finishedCount << "\n";
    std::cout << "----------------------------------------\n";
}