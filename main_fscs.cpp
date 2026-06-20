#include "Scheduler.h"
#include "Process.h"
#include "PrintCommand.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

// Global flag to control the main loop
std::atomic<bool> shouldExit(false);

int main() {
    // Create scheduler with 4 cores
    Scheduler scheduler(4);
    
    // Create 10 processes, each with 100 print commands
    for (int i = 1; i <= 10; ++i) {
        // Build process name with proper string concatenation
        std::string processName = "process";
        if (i < 10) {
            processName += "0";
        }
        processName += std::to_string(i);
        
        auto process = std::make_shared<Process>(i, processName);
        
        // Add 100 print commands
        for (int j = 1; j <= 100; ++j) {
            std::string message = "Print command #" + std::to_string(j) + 
                                 " from " + processName;
            process->addCommand(std::make_shared<PrintCommand>(message, true));
        }
        
        scheduler.addProcess(process);
    }
    
    // Start the scheduler
    scheduler.start();
    std::cout << "========================================" << std::endl;
    std::cout << "  OS Emulator started with FCFS scheduler" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  screen -ls  - Show scheduler status" << std::endl;
    std::cout << "  exit        - Close the emulator" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Command loop
    std::string command;
    while (!shouldExit) {
        std::cout << "> ";
        std::getline(std::cin, command);
        
        if (command == "screen -ls") {
            scheduler.printStatus();
        } else if (command == "exit") {
            shouldExit = true;
            break;
        } else if (command.empty()) {
            continue;
        } else {
            std::cout << "Unknown command. Use 'screen -ls' or 'exit'" << std::endl;
        }
        
        // Check if all processes are finished (but don't exit automatically)
        if (scheduler.allProcessesFinished()) {
            std::cout << "\n========================================" << std::endl;
            std::cout << "  All processes have finished!" << std::endl;
            std::cout << "========================================" << std::endl;
            scheduler.printStatus();
            std::cout << "Type 'exit' to close the emulator" << std::endl;
            // Don't automatically exit - let user type 'exit' manually
        }
    }
    
    // Clean up
    scheduler.stop();
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Emulator closed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}