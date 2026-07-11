#pragma once
#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include "OSProcess.h"

struct MemoryBlock {
    int startAddress;
    int size;
    bool isFree;
    std::shared_ptr<OSProcess> process;
    
    MemoryBlock(int start, int sz) 
        : startAddress(start), size(sz), isFree(true), process(nullptr) {}
};

class MemoryManager {
private:
    int totalMemory;
    int frameSize;
    int memPerProcess;
    std::vector<MemoryBlock> blocks;
    mutable std::mutex memoryMutex;
    
public:
    MemoryManager(int totalMem, int frameSize, int memPerProc);
    
    bool allocateMemory(std::shared_ptr<OSProcess> process);
    void releaseMemory(std::shared_ptr<OSProcess> process);
    int calculateExternalFragmentation();
    int getProcessCount();
    std::string getMemoryMap();
    std::string getASCIIMemoryMap();
    void mergeFreeBlocks();
    void printMemoryStatus();
    int getTotalMemory() const { return totalMemory; }
};