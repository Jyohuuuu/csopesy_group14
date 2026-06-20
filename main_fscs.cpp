#include "Scheduler.h"
#include "Process.h"
#include "PrintCommand.h"
#include "ConsoleSync.h"
#include <iostream>
#include <string>
#include <atomic>
#include <cstdio> // std::remove

std::atomic<bool> shouldExit(false);

int main() {
    // Clean previous run files
    for (int i = 1; i <= 10; ++i) {
        std::string processName = "process";
        if (i < 10) processName += "0";
        processName += std::to_string(i);

        std::string filename = "process_" + std::to_string(i) + "_" + processName + ".txt";
        std::remove(filename.c_str());
    }

    Scheduler scheduler(4);

    // 10 processes, each with 100 print commands
    for (int i = 1; i <= 10; ++i) {
        std::string processName = "process";
        if (i < 10) processName += "0";
        processName += std::to_string(i);

        auto process = std::make_shared<Process>(i, processName);

        for (int j = 1; j <= 100; ++j) {
            std::string msg = "Hello world from " + processName + "!";
            process->addCommand(std::make_shared<PrintCommand>(msg, true));
        }

        scheduler.addProcess(process);
    }

    scheduler.start();

    {
        std::lock_guard<std::mutex> lock(g_outputMutex);
        std::cout << "========================================\n";
        std::cout << "  OS Emulator started with FCFS scheduler\n";
        std::cout << "========================================\n";
        std::cout << "Commands:\n";
        std::cout << "  screen -ls  - Show scheduler status\n";
        std::cout << "  exit        - Close the emulator\n";
        std::cout << "========================================\n\n";
    }

    std::string command;
    while (!shouldExit) {
        {
            std::lock_guard<std::mutex> lock(g_outputMutex);
            std::cout << "> ";
        }

        std::getline(std::cin, command);

        if (command == "screen -ls") {
            scheduler.printStatus();
        } else if (command == "exit") {
            shouldExit = true;
            break;
        } else if (!command.empty()) {
            std::lock_guard<std::mutex> lock(g_outputMutex);
            std::cout << "Unknown command. Use 'screen -ls' or 'exit'\n";
        }

        if (scheduler.allProcessesFinished()) {
            std::lock_guard<std::mutex> lock(g_outputMutex);
            std::cout << "\n========================================\n";
            std::cout << "  All processes have finished!\n";
            std::cout << "========================================\n";
            std::cout << "Type 'exit' to close the emulator\n";
        }
    }

    scheduler.stop();

    {
        std::lock_guard<std::mutex> lock(g_outputMutex);
        std::cout << "\n========================================\n";
        std::cout << "  Emulator closed\n";
        std::cout << "========================================\n";
    }

    return 0;
}