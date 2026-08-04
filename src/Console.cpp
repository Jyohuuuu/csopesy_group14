#include "../include/Console.h"
#include "../include/Scheduler.h"
#include "../include/OSProcess.h"
#include "../include/PrintCommand.h"
#include "../include/ConsoleSync.h"
#include "../include/FileUtils.h"
#include "../include/ProcessInstructions.h"
#include "../include/MemoryManager.h"
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
                std::string sizeStr;
                iss >> sizeStr;
                
                if (name.empty()) {
                    std::cout << "Usage: screen -s <process_name> [memory_size]\n";
                    std::cout << "Memory size must be between 64 and 32768 bytes (power of 2)\n";
                    continue;
                }
                
                if (sizeStr.empty()) {
                    // Prompt for memory size
                    std::cout << "Enter memory size (64-32768, power of 2): ";
                    std::getline(std::cin, sizeStr);
                    // Trim
                    sizeStr.erase(0, sizeStr.find_first_not_of(" \t\n\r\f\v"));
                    sizeStr.erase(sizeStr.find_last_not_of(" \t\n\r\f\v") + 1);
                }
                
                try {
                    int memorySize = std::stoi(sizeStr);
                    if (isValidMemorySize(memorySize)) {
                        handleScreenCreate(name, memorySize);
                    } else {
                        std::cout << "Invalid memory allocation. Must be between 64 and 32768 bytes, power of 2.\n";
                    }
                } catch (...) {
                    std::cout << "Invalid memory size. Must be a number.\n";
                }
            }
            else if (subcmd == "-c") {
                std::string name, sizeStr, instructions;
                iss >> name >> sizeStr;
                // Read remaining as instructions
                std::string line;
                std::getline(iss, line);
                instructions = line;

                // Trim whitespace first
                instructions.erase(0, instructions.find_first_not_of(" \t\n\r\f\v"));
                instructions.erase(instructions.find_last_not_of(" \t\n\r\f\v") + 1);

                // Now the quotes are actually at front()/back()
                if (!instructions.empty() && instructions.front() == '"') {
                    instructions = instructions.substr(1);
                }
                if (!instructions.empty() && instructions.back() == '"') {
                    instructions.pop_back();
                }

                // Trim again in case removing quotes exposed more whitespace
                instructions.erase(0, instructions.find_first_not_of(" \t\n\r\f\v"));
                instructions.erase(instructions.find_last_not_of(" \t\n\r\f\v") + 1);
                
                if (name.empty() || sizeStr.empty() || instructions.empty()) {
                    std::cout << "Usage: screen -c <process_name> <memory_size> \"<instructions>\"\n";
                    std::cout << "Instructions: 1-50 semicolon-separated commands\n";
                    continue;
                }
                
                try {
                    int memorySize = std::stoi(sizeStr);
                    if (isValidMemorySize(memorySize)) {
                        handleScreenCustom(name, memorySize, instructions);
                    } else {
                        std::cout << "Invalid memory allocation. Must be between 64 and 32768 bytes, power of 2.\n";
                    }
                } catch (...) {
                    std::cout << "Invalid memory size. Must be a number.\n";
                }
            }
            else if (subcmd == "-r") {
                std::string name;
                iss >> name;
                handleScreenAttach(name);
            }
            else {
                std::cout << "Usage: screen -ls, screen -s <name> [size], screen -c <name> <size> \"<instructions>\", screen -r <name>\n";
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
        else if (cmd == "vmstat") {
            handleVMStat();
        }
        else if (!cmd.empty()) {
            std::cout << "Unknown command. Type 'initialize' first or 'exit' to quit.\n";
        }
    }
}

void Console::printMainMenu() {
    std::lock_guard<std::mutex> lock(g_outputMutex);
    std::cout << "\n========================================\n";
    std::cout << "  OS Emulator v2.0  Last Updated: 2026-8-3\n";
    std::cout << "========================================\n";
    std::cout << "Commands:\n";
    std::cout << "  initialize      - Load config.txt\n";
    std::cout << "  exit            - Close emulator\n";
    std::cout << "  screen -ls      - List processes\n";
    std::cout << "  screen -s NAME [SIZE] - Create process with memory\n";
    std::cout << "  screen -c NAME SIZE \"INSTR\" - Create with custom instructions\n";
    std::cout << "  screen -r NAME  - Attach to process\n";
    std::cout << "  scheduler-start - Start process generation\n";
    std::cout << "  scheduler-stop  - Stop process generation\n";
    std::cout << "  report-util     - Generate CPU utilization report\n";
    std::cout << "  vmstat          - Show virtual memory statistics\n";
    std::cout << "========================================\n";
}

void Console::handleInitialize() {
    Config config;
    if (config.loadFromFile("config.txt")) {
        initialized = true;
        scheduler = std::make_unique<Scheduler>(config);
        scheduler->start();
        
        std::cout << "\n========================================\n";
        std::cout << "  SYSTEM INITIALIZATION COMPLETE\n";
        std::cout << "========================================\n";
        
        std::cout << "\nCPU Configuration:\n";
        std::cout << "  Number of CPUs: " << config.numCpu << "\n";
        std::cout << "  Scheduler: " << config.scheduler << "\n";
        std::cout << "  Quantum Cycles: " << config.quantumCycles << "\n";
        std::cout << "  Batch Process Frequency: " << config.batchProcessFreq << "\n";
        std::cout << "  Min Instructions: " << config.minIns << "\n";
        std::cout << "  Max Instructions: " << config.maxIns << "\n";
        std::cout << "  Delay Per Execution: " << config.delayPerExec << "\n";
        
        std::cout << "\nMemory Configuration:\n";
        std::cout << "  Total Memory: " << config.maxOverallMem << " bytes (" 
                  << config.maxOverallMem / 1024 << " KB)\n";
        std::cout << "  Frame Size: " << config.memPerFrame << " bytes\n";
        std::cout << "  Min Memory per Process: " << config.minMemPerProc << " bytes (" 
                  << config.minMemPerProc / 1024 << " KB)\n";
        std::cout << "  Max Memory per Process: " << config.maxMemPerProc << " bytes (" 
                  << config.maxMemPerProc / 1024 << " KB)\n";
        
        std::cout << "\n========================================\n";
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

void Console::handleScreenCreate(const std::string& name, int memorySize) {
    if (!initialized) {
        std::cout << "Please run 'initialize' first\n";
        return;
    }
    
    if (name.empty()) {
        std::cout << "Usage: screen -s <process_name> <memory_size>\n";
        return;
    }
    
    // Avoid creating a duplicate process name (also avoids page-table collisions)
    if (scheduler && scheduler->findProcessByName(name)) {
        std::cout << "Process '" << name << "' already exists. Use 'screen -r " << name << "' to attach.\n";
        return;
    }
    
    static int pidCounter = 1000;
    auto process = std::make_shared<OSProcess>(pidCounter++, name);
    process->setMemorySize(memorySize);
    
    if (scheduler) {
        process->setMemoryManager(std::shared_ptr<MemoryManager>(scheduler->getMemoryManager(), [](MemoryManager*){}));
    }
    
    std::vector<Instruction> instructions = ProcessGenerator::generateInstructions(
        scheduler->getMinIns(), 
        scheduler->getMaxIns(), 
        name,
        memorySize
    );
    
    for (const auto& instr : instructions) {
        process->addInstruction(instr);
    }
    
    scheduler->addProcess(process);
    
    // Move into the process screen
    clearScreen();
    inScreenSession = true;
    currentScreenProcess = name;
    
    std::cout << "=== Attached to " << name << " ===\n";
    std::cout << "PID: " << process->getPID() << "\n";
    std::cout << "Memory: " << memorySize << " bytes\n";
    std::cout << "Instructions: " << instructions.size() << "\n";
    std::cout << "Commands: process-smi, vmstat, exit\n";
    std::cout << "Type 'exit' to detach from process\n\n";
}

void Console::handleScreenCustom(const std::string& name, int memorySize, const std::string& instructions) {
    if (!initialized) {
        std::cout << "Please run 'initialize' first\n";
        return;
    }
    
    if (!isValidMemorySize(memorySize)) {
        std::cout << "Invalid memory allocation. Must be between 64 and 32768 bytes, power of 2.\n";
        return;
    }
    
    if (scheduler && scheduler->findProcessByName(name)) {
        std::cout << "Process '" << name << "' already exists. Use 'screen -r " << name << "' to attach.\n";
        return;
    }
    
    std::vector<Instruction> parsedInstructions = parseCustomInstructions(instructions);
    if (parsedInstructions.empty()) {
        std::cout << "Invalid command. No valid instructions found.\n";
        return;
    }
    if (parsedInstructions.size() > 50) {
        std::cout << "Too many instructions. Maximum 50.\n";
        return;
    }
    
    static int pidCounter = 1000;
    auto process = std::make_shared<OSProcess>(pidCounter++, name);
    process->setMemorySize(memorySize);
    
    if (scheduler) {
        process->setMemoryManager(std::shared_ptr<MemoryManager>(scheduler->getMemoryManager(), [](MemoryManager*){}));
    }
    
    for (const auto& instr : parsedInstructions) {
        process->addInstruction(instr);
    }
    
    scheduler->addProcess(process);
    
    // Move into the process screen
    clearScreen();
    inScreenSession = true;
    currentScreenProcess = name;
    
    std::cout << "=== Attached to " << name << " ===\n";
    std::cout << "PID: " << process->getPID() << "\n";
    std::cout << "Memory: " << memorySize << " bytes\n";
    std::cout << "Instructions: " << parsedInstructions.size() << "\n";
    std::cout << "Commands: process-smi, vmstat, exit\n";
    std::cout << "Type 'exit' to detach from process\n\n";
}

std::vector<Instruction> Console::parseCustomInstructions(const std::string& instructions) {
    std::vector<Instruction> result;
    std::stringstream ss(instructions);
    std::string instrStr;
    
    while (std::getline(ss, instrStr, ';')) {
        instrStr.erase(0, instrStr.find_first_not_of(" \t\n\r\f\v"));
        instrStr.erase(instrStr.find_last_not_of(" \t\n\r\f\v") + 1);
        
        if (instrStr.empty()) continue;
        
        bool valid = false;
        Instruction inst = parseSingleInstruction(instrStr, valid);
        if (!valid) {
            std::cout << "Warning: could not parse instruction: \"" << instrStr << "\" (skipped)\n";
            continue;
        }
        result.push_back(inst);
    }
    
    return result;
}

Instruction Console::parseSingleInstruction(const std::string& instr, bool& valid) {
    valid = true;
    std::stringstream ss(instr);
    std::string command;
    ss >> command;
    
    std::vector<std::string> params;
    std::string param;
    while (ss >> param) {
        params.push_back(param);
    }
    
    if (command == "PRINT") {
        std::string message;
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) message += " ";
            message += params[i];
        }
        if (message.size() >= 2 && message.front() == '"' && message.back() == '"') {
            message = message.substr(1, message.size() - 2);
        }
        return Instruction(InstructionType::PRINT, {message});
    } else if (command == "DECLARE") {
        if (params.size() >= 2) {
            return Instruction(InstructionType::DECLARE, {params[0], params[1]});
        }
    } else if (command == "ADD") {
        if (params.size() >= 3) {
            return Instruction(InstructionType::ADD, {params[0], params[1], params[2]});
        }
    } else if (command == "SUBTRACT") {
        if (params.size() >= 3) {
            return Instruction(InstructionType::SUBTRACT, {params[0], params[1], params[2]});
        }
    } else if (command == "SLEEP") {
        if (params.size() >= 1) {
            return Instruction(InstructionType::SLEEP, {params[0]});
        }
    } else if (command == "READ") {
        if (params.size() >= 2) {
            return Instruction(InstructionType::READ, {params[0], params[1]});
        }
    } else if (command == "WRITE") {
        if (params.size() >= 2) {
            return Instruction(InstructionType::WRITE, {params[0], params[1]});
        }
    }
    
    // Nothing matched, or matched but wrong arg count
    valid = false;
    return Instruction(InstructionType::PRINT, {""});
}

bool Console::isValidMemorySize(int size) const {
    if (size < 64 || size > 32768) return false;
    return (size & (size - 1)) == 0;
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
    
    std::shared_ptr<OSProcess> process = scheduler->findProcessByName(name);
    
    if (process) {
        if (process->hasMemoryViolation()) {
            std::cout << "Process " << name << " shut down due to memory access violation error ";
            std::cout << "that occurred at " << process->getViolationTimeString() << ". ";
            std::cout << "0x" << std::hex << process->getViolationAddress() << std::dec << " invalid.\n";
            return;
        }
        
        inScreenSession = true;
        currentScreenProcess = name;
        std::cout << "\n=== Attached to " << name << " ===\n";
        if (process->isFinished()) {
            std::cout << "(Process has already finished.)\n";
        }
        std::cout << "Commands: process-smi, vmstat, exit\n";
        std::cout << "Type 'exit' to detach from process\n\n";
    } else {
        std::cout << "Process " << name << " not found\n";
    }
}
void Console::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
void Console::processScreenCommand(const std::string& command) {
    std::string cmd = command;
    cmd.erase(0, cmd.find_first_not_of(" \t\n\r\f\v"));
    cmd.erase(cmd.find_last_not_of(" \t\n\r\f\v") + 1);
    
    if (cmd == "exit") {
        inScreenSession = false;
        currentScreenProcess = "";
        clearScreen();
        printMainMenu();
        return;
    }
    
    if (cmd == "process-smi") {
        if (!scheduler) {
            std::cout << "Scheduler not available\n";
            inScreenSession = false;
            currentScreenProcess = "";
            return;
        }
        
        std::shared_ptr<OSProcess> process = scheduler->findProcessByName(currentScreenProcess);
        
        if (!process) {
            std::cout << "Process " << currentScreenProcess << " not found\n";
            inScreenSession = false;
            currentScreenProcess = "";
            return;
        }
        
        std::cout << "\n========================================\n";
        std::cout << "  PROCESS INFORMATION\n";
        std::cout << "========================================\n";
        std::cout << "Process Name: " << process->getName() << "\n";
        std::cout << "PID: " << process->getPID() << "\n";
        std::cout << "Memory Size: " << process->getMemorySize() << " bytes\n";
        std::cout << "Memory Used: " << process->getMemoryUsed() << " bytes\n";
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
        std::cout << "Variables: " << process->getSymbolTable().getSize() 
                  << " / 32 (max)\n";
        std::cout << "Started: " << process->getStartTimeString() << "\n";
        
        if (process->hasMemoryViolation()) {
            std::cout << "\n!!! MEMORY ACCESS VIOLATION !!!\n";
            std::cout << "Address: 0x" << std::hex << process->getViolationAddress() << std::dec << "\n";
            std::cout << "Time: " << process->getViolationTimeString() << "\n";
        }
        
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
    
    if (cmd == "vmstat") {
        handleVMStat();
        return;
    }
    
    if (cmd.find("screen") != std::string::npos) {
        std::cout << "Cannot use 'screen' commands while attached to a process.\n";
        std::cout << "Type 'exit' to detach first.\n";
        return;
    }
    
    if (!cmd.empty()) {
        std::cout << "Unknown command. Use 'process-smi', 'vmstat', or 'exit'\n";
    }
}

void Console::handleVMStat() {
    if (!scheduler) {
        std::cout << "Scheduler not initialized\n";
        return;
    }
    
    MemoryManager* memManager = scheduler->getMemoryManager();
    if (!memManager) {
        std::cout << "Memory manager not available\n";
        return;
    }
    
    std::cout << "\n========================================\n";
    std::cout << "  VIRTUAL MEMORY STATISTICS\n";
    std::cout << "========================================\n";
    
    // Get memory statistics
    int totalMem = memManager->getTotalMemory();
    int usedMem = memManager->getUsedMemory();
    int freeMem = totalMem - usedMem;
    
    std::cout << "Total memory: " << totalMem << " bytes (" << (totalMem/1024) << " KB)\n";
    std::cout << "Used memory: " << usedMem << " bytes (" << (usedMem/1024) << " KB)\n";
    std::cout << "Free memory: " << freeMem << " bytes (" << (freeMem/1024) << " KB)\n";
    std::cout << "Frame size: " << memManager->getFrameSize() << " bytes\n";
    
    // Page statistics
    std::cout << "Num paged in: " << memManager->getNumPagesPagedIn() << "\n";
    std::cout << "Num paged out: " << memManager->getNumPagesPagedOut() << "\n";
    
    // Process information
    std::cout << "Processes in memory: " << memManager->getProcessCount() << "\n";
    std::cout << "Total memory allocations: " << memManager->getTotalMemoryAllocations() << "\n";
    std::cout << "Total memory deallocations: " << memManager->getTotalMemoryDeallocations() << "\n";
    
    // CPU statistics
    float cpuUtil = scheduler->getCPUUtilization();
    std::cout << "CPU Utilization: " << cpuUtil << "%\n";
    std::cout << "Total instructions executed: " << scheduler->getTotalInstructionsExecuted() << "\n";
    
    std::cout << "========================================\n";
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