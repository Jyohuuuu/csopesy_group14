#include "Scheduler.h"
#include "Process.h"
#include "PrintCommand.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main() {
    // Create scheduler with 4 cores
    Scheduler scheduler(4);
    
    // Create 10 processes, each with 100 print commands, temp
    for (int i = 1; i <= 10; ++i) {
        std::string processName = "process";
        if (i < 10) {
            processName += "0";
        }
        processName += std::to_string(i);
        
        auto process = std::make_shared<Process>(i, processName);
        
        for (int j = 1; j <= 100; ++j) {
            std::string message = "Print command #" + std::to_string(j) + 
                                 " from " + processName;
            process->addCommand(std::make_shared<PrintCommand>(message, true));
        }
        
        scheduler.addProcess(process);
    }
    
    scheduler.start();
    std::cout << "OS Emulator started with FCFS scheduler" << std::endl;
    std::cout << "Type 'screen -ls' to check status, 'exit' to quit" << std::endl;
    
    // Command loop
    std::string command;
    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, command);
        
        if (command == "screen -ls") {
            scheduler.printStatus();
        } else if (command == "exit") {
            break;
        } else if (command.empty()) {
            continue;
        } else {
            std::cout << "Unknown command. Use 'screen -ls' or 'exit'" << std::endl;
        }
        

        if (scheduler.allProcessesFinished()) {
            std::cout << "\nAll processes finished!" << std::endl;
            scheduler.printStatus();
            std::cout << "All processes complete. Exiting..." << std::endl;
            break;
        }
    }
    
    scheduler.stop();
    std::cout << "Emulator closed" << std::endl;
    
    return 0;
}