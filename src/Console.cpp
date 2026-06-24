#include "../include/Console.h"
#include "../include/Scheduler.h"
#include "../include/OSProcess.h"
#include "../include/PrintCommand.h"
#include "../include/ConsoleSync.h"
#include "../include/FileUtils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <random>

Console::Console() : initialized(false), inScreenSession(false) {}
Console::~Console() {}

void Console::run() {
    std::string command;
    printMainMenu();
    
    while (true) {
        if (!inScreenSession) {
            std::cout << "\n> ";
        }
        std::getline(std::cin, command);
        
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
    std::cout << "  OS Emulator v1.0\n";
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
    static int pidCounter = 1000;
    auto process = std::make_shared<Process>(pidCounter++, name);
    for (int i = 0; i < 50; i++) {
        std::string msg = "Hello world from " + name + "!";
        process->addCommand(std::make_shared<PrintCommand>(msg, true));
    }
    scheduler->addProcess(process);
    std::cout << "Process " << name << " created with PID " << process->getPID() << "\n";
}

void Console::handleScreenAttach(const std::string& name) {
    if (!initialized) {
        std::cout << "Please run 'initialize' first\n";
        return;
    }
    auto process = scheduler->findProcessByName(name);
    if (process && !process->isFinished()) {
        inScreenSession = true;
        currentScreenProcess = name;
        runScreenSession(name);
    }
    else {
        std::cout << "Process " << name << " not found or already finished\n";
    }
}

void Console::runScreenSession(const std::string& name) {
    std::cout << "\n=== Attached to " << name << " ===\n";
    std::cout << "Commands: process-smi, exit\n\n";
}

void Console::processScreenCommand(const std::string& command) {
    if (command == "process-smi") {
        auto process = scheduler->findProcessByName(currentScreenProcess);
        if (process) {
            std::cout << "Process: " << process->getName() << "\n";
            std::cout << "PID: " << process->getPID() << "\n";
            std::cout << "State: ";
            if (process->isFinished()) {
                std::cout << "FINISHED\n";
                std::cout << "Ended: " << process->getEndTimeString() << "\n";
            } else {
                std::cout << "RUNNING\n";
            }
            std::cout << "Instructions: " << process->getCommandCounter() 
                      << " / " << process->getTotalCommands() << "\n";
            std::cout << "Started: " << process->getStartTimeString() << "\n";
        }
    }
    else if (command == "exit") {
        inScreenSession = false;
        currentScreenProcess = "";
        std::cout << "Returning to main menu...\n";
    }
    else {
        std::cout << "Unknown screen command. Use 'process-smi' or 'exit'\n";
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