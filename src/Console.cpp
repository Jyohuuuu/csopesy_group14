#include "../include/Console.h"
#include "../include/Scheduler.h"
#include "../include/OSProcess.h"
#include "../include/PrintCommand.h"
#include "../include/ConsoleSync.h"
#include "../include/FileUtils.h"
#include "../include/ProcessInstructions.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <random>
#include <chrono>
#include <thread>

Console::Console() : initialized(false), inScreenSession(false) {}
Console::~Console() {}

void Console::run() {
    std::string command;
    printMainMenu();
    
    while (true) {
        if (inScreenSession) {
            std::cout << currentScreenProcess << "> ";
        } else {
            std::cout << "\n> ";
        }
        std::cout.flush();
        
        if (!std::getline(std::cin, command)) {
            break;
        }
        
        if (inScreenSession) {
            processScreenCommand(command);
            continue;
        }
        
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "initialize") {
            handleInitialize();
        }
        else if (cmd == "exit") {
            handleExit();
            break;
        }
        else if (cmd == "screen") {
            std::string subcmd;
            iss >> subcmd;
            if (subcmd == "-ls") {
                handleScreenLS();
            }
            else if (subcmd == "-s") {
                std::string name;
                iss >> name;
                handleScreenCreate(name);
            }
            else if (subcmd == "-r") {
                std::string name;
                iss >> name;
                handleScreenAttach(name);
            }
            else {
                std::cout << "Usage: screen -ls, screen -s <name>, screen -r <name>\n";
            }
        }
        else if (cmd == "scheduler-start") {
            handleSchedulerStart();
        }
        else if (cmd == "scheduler-stop") {
            handleSchedulerStop();
        }
        else if (cmd == "report-util") {
            handleReportUtil();
        }
        else if (!cmd.empty()) {
            std::cout << "Unknown command. Type 'initialize' first or 'exit' to quit.\n";
        }
    }
}

void Console::printMainMenu() {
    std::lock_guard<std::mutex> lock(g_outputMutex);
    std::cout << "\n========================================\n";
    std::cout << "  OS Emulator v1.02  Last Updated: 2026-6-24\n";
    std::cout << "========================================\n";
    std::cout << "Commands:\n";
    std::cout << "  initialize      - Load config.txt\n";
    std::cout << "  exit            - Close emulator\n";
    std::cout << "  screen -ls      - List processes\n";
    std::cout << "  screen -s NAME  - Create process\n";
    std::cout << "  screen -r NAME  - Attach to process\n";
    std::cout << "  scheduler-start - Start process generation\n";
    std::cout << "  scheduler-stop  - Stop process generation\n";
    std::cout << "  report-util     - Generate CPU utilization report\n";
    std::cout << "========================================\n";
}

void Console::handleInitialize() {
    Config config;
    if (config.loadFromFile("config.txt")) {
        initialized = true;
        scheduler = std::make_unique<Scheduler>(config);
        scheduler->start();
        std::cout << "Initialized with " << config.numCpu << " CPUs, "
                  << config.scheduler << " scheduler\n";
    }
}

void Console::handleExit() {
    if (scheduler) {
        scheduler->stop();
    }
    std::cout << "Exiting OS Emulator...\n";
}

void Console::handleScreenLS() {
    if (!initialized) {
        std::cout << "Please run 'initialize' first\n";
        return;
    }
    scheduler->printStatus();
}

void Console::handleScreenCreate(const std::string& name) {
    if (!initialized) {
        std::cout << "Please run 'initialize' first\n";
        return;
    }
    
    if (name.empty()) {
        std::cout << "Usage: screen -s <process_name>\n";
        return;
    }
    
    static int pidCounter = 1000;
    auto process = std::make_shared<OSProcess>(pidCounter++, name);
    
    std::vector<Instruction> instructions = ProcessGenerator::generateInstructions(
        scheduler->getMinIns(), 
        scheduler->getMaxIns(), 
        name
    );
    
    for (const auto& instr : instructions) {
        process->addInstruction(instr);
    }
    
    scheduler->addProcess(process);
    std::cout << "Process " << name << " created with PID " << process->getPID() << "\n";
    std::cout << "Instructions: " << instructions.size() << "\n";
    std::cout << "Use 'screen -r " << name << "' to attach\n";
}

void Console::handleScreenAttach(const std::string& name) {
    if (!initialized) {
        std::cout << "Please run 'initialize' first\n";
        return;
    }
    
    if (!scheduler) {
        std::cout << "Scheduler not initialized\n";
        return;
    }
    
    // Get a copy of the process with proper locking
    std::shared_ptr<OSProcess> process = scheduler->findProcessByName(name);
    
    if (process) {
        // Check if finished (lock is released, but shared_ptr keeps process alive)
        if (!process->isFinished()) {
            inScreenSession = true;
            currentScreenProcess = name;
            std::cout << "\n=== Attached to " << name << " ===\n";
            std::cout << "Commands: process-smi, exit\n";
            std::cout << "Type 'exit' to detach from process\n\n";
        } else {
            std::cout << "Process " << name << " has already finished\n";
        }
    } else {
        std::cout << "Process " << name << " not found\n";
    }
}

void Console::processScreenCommand(const std::string& command) {
    std::string cmd = command;
    cmd.erase(0, cmd.find_first_not_of(" \t\n\r\f\v"));
    cmd.erase(cmd.find_last_not_of(" \t\n\r\f\v") + 1);
    
    if (cmd == "exit") {
        inScreenSession = false;
        currentScreenProcess = "";
        std::cout << "Detached from process. Returning to main menu...\n";
        return;
    }
    
    if (cmd == "process-smi") {
        if (!scheduler) {
            std::cout << "Scheduler not available\n";
            inScreenSession = false;
            currentScreenProcess = "";
            return;
        }
        
        // Get a copy of the process with proper locking
        std::shared_ptr<OSProcess> process = scheduler->findProcessByName(currentScreenProcess);
        
        if (!process) {
            std::cout << "Process " << currentScreenProcess << " not found\n";
            inScreenSession = false;
            currentScreenProcess = "";
            return;
        }
        
        // Now safely display process info - the shared_ptr keeps it alive
        std::cout << "\n========================================\n";
        std::cout << "  PROCESS INFORMATION\n";
        std::cout << "========================================\n";
        std::cout << "Process Name: " << process->getName() << "\n";
        std::cout << "PID: " << process->getPID() << "\n";
        std::cout << "State: ";
        
        if (process->isFinished()) {
            std::cout << "FINISHED\n";
            std::cout << "Ended: " << process->getEndTimeString() << "\n";
        } else if (process->isWaiting()) {
            std::cout << "SLEEPING (" << process->getWaitTicks() << " ticks remaining)\n";
        } else {
            OSProcess::ProcessState state = process->getState();
            if (state == OSProcess::RUNNING) {
                std::cout << "RUNNING\n";
            } else if (state == OSProcess::READY) {
                std::cout << "READY\n";
            } else {
                std::cout << "UNKNOWN (" << state << ")\n";
            }
        }
        
        std::cout << "Instructions: " << process->getCommandCounter() 
                  << " / " << process->getTotalCommands() << "\n";
        std::cout << "Started: " << process->getStartTimeString() << "\n";
        
        std::cout << "\nProcess Logs (PRINT outputs):\n";
        std::cout << "----------------------------------------\n";
        const auto& logs = process->getOutputLogs();
        if (logs.empty()) {
            std::cout << "(No PRINT outputs yet)\n";
        } else {
            size_t count = 0;
            for (const auto& log : logs) {
                std::cout << "  " << log << "\n";
                count++;
                if (count >= 50 && count < logs.size()) {
                    std::cout << "  ... (" << (logs.size() - count) << " more logs)\n";
                    break;
                }
            }
        }
        std::cout << "========================================\n\n";
        return;
    }
    
    if (cmd.find("screen") != std::string::npos) {
        std::cout << "Cannot use 'screen' commands while attached to a process.\n";
        std::cout << "Type 'exit' to detach first.\n";
        return;
    }
    
    if (!cmd.empty()) {
        std::cout << "Unknown command. Use 'process-smi' or 'exit'\n";
    }
}

void Console::handleSchedulerStart() {
    if (!initialized) {
        std::cout << "Please run 'initialize' first\n";
        return;
    }
    scheduler->startGenerating();
    std::cout << "Scheduler started generating processes...\n";
}

void Console::handleSchedulerStop() {
    if (scheduler) {
        scheduler->stopGenerating();
    }
    std::cout << "Scheduler stopped generating processes\n";
}

void Console::handleReportUtil() {
    if (!initialized) {
        std::cout << "Please run 'initialize' first\n";
        return;
    }
    scheduler->saveUtilizationReport("csopesy-log.txt");
    std::cout << "CPU utilization report saved to csopesy-log.txt\n";
}