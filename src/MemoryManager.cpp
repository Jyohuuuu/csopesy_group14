#include "../include/MemoryManager.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>

MemoryManager::MemoryManager(int totalMem, int frameSize, int memPerProc)
    : totalMemory(totalMem), frameSize(frameSize), memPerProcess(memPerProc) {
    blocks.push_back(MemoryBlock(0, totalMemory));
}

bool MemoryManager::allocateMemory(std::shared_ptr<OSProcess> process) {
    if (!process) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    for (size_t i = 0; i < blocks.size(); i++) {
        if (blocks[i].isFree && blocks[i].size >= memPerProcess) {
            if (blocks[i].size > memPerProcess) {
                int newStart = blocks[i].startAddress + memPerProcess;
                int newSize = blocks[i].size - memPerProcess;
                MemoryBlock newBlock(newStart, newSize);
                blocks.insert(blocks.begin() + i + 1, newBlock);
                blocks[i].size = memPerProcess;
            }
            
            blocks[i].isFree = false;
            blocks[i].process = process;
            
            return true;
        }
    }
    
    return false;
}

void MemoryManager::releaseMemory(std::shared_ptr<OSProcess> process) {
    if (!process) return;
    
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    int freedCount = 0;
    for (auto& block : blocks) {
        if (!block.isFree && block.process == process) {
            block.isFree = true;
            block.process = nullptr;
            freedCount++;
        }
    }
    if (freedCount > 1) {
        std::cerr << "[WARNING] releaseMemory: process " << process->getName()
                   << " held " << freedCount << " blocks (expected 1) - "
                   << "this indicates the process was allocated memory more than once\n";
    }
    mergeFreeBlocks();
}

void MemoryManager::mergeFreeBlocks() {
    for (size_t i = 0; i + 1 < blocks.size(); i++) {
        if (blocks[i].isFree && blocks[i + 1].isFree) {
            blocks[i].size += blocks[i + 1].size;
            blocks.erase(blocks.begin() + i + 1);
            i--;
        }
    }
}

int MemoryManager::calculateExternalFragmentation() {
    std::lock_guard<std::mutex> lock(memoryMutex);
    int fragmentation = 0;
    for (const auto& block : blocks) {
        if (block.isFree) {
            fragmentation += block.size;
        }
    }
    return fragmentation;
}

int MemoryManager::getProcessCount() {
    std::lock_guard<std::mutex> lock(memoryMutex);
    int count = 0;
    for (const auto& block : blocks) {
        if (!block.isFree && block.process) {
            count++;
        }
    }
    return count;
}

std::string MemoryManager::getMemoryMap() {
    std::lock_guard<std::mutex> lock(memoryMutex);
    std::stringstream ss;
    
    ss << "--- Memory Map ---\n";
    ss << "Total Memory: " << totalMemory << " bytes\n";
    ss << "Frame Size: " << frameSize << " bytes\n";
    ss << "Process Memory: " << memPerProcess << " bytes\n\n";
    
    for (const auto& block : blocks) {
        if (!block.isFree && block.process) {
            ss << std::setw(8) << block.startAddress 
               << " | " << std::setw(6) << block.size 
               << " | " << block.process->getName() << "\n";
        } else {
            ss << std::setw(8) << block.startAddress 
               << " | " << std::setw(6) << block.size 
               << " | FREE\n";
        }
    }
    
    ss << "\n---end--- " << totalMemory << "\n";
    return ss.str();
}

std::string MemoryManager::getASCIIMemoryMap() {
    std::lock_guard<std::mutex> lock(memoryMutex);
    std::stringstream ss;

    ss << "----end---- = " << totalMemory << "\n\n";
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        const MemoryBlock& block = *it;
        if (!block.isFree && block.process) {
            int upperBound = block.startAddress + block.size;
            int lowerBound = block.startAddress;
            ss << upperBound << "\n";
            ss << block.process->getName() << "\n";
            ss << lowerBound << "\n\n";
        }
    }

    ss << "----start----- = 0\n";
    return ss.str();
}

void MemoryManager::printMemoryStatus() {
    std::lock_guard<std::mutex> lock(memoryMutex);
    std::cout << "\n--- Memory Status ---\n";
    std::cout << "Total memory: " << totalMemory << " bytes\n";
    std::cout << "Processes in memory: " << getProcessCount() << "\n";
    std::cout << "External fragmentation: " << calculateExternalFragmentation() << " bytes ("
              << calculateExternalFragmentation() / 1024 << " KB)\n";
    std::cout << "Memory map:\n";
    std::cout << getMemoryMap();
}