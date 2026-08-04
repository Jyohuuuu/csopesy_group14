#pragma once
#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "OSProcess.h"

struct PageTableEntry {
    bool valid = false;
    int frameIndex = -1;
    bool referenced = false;
    bool modified = false;
    int virtualPageNum = -1;
    std::string processName; 
};

struct PhysicalFrame {
    bool occupied = false;
    std::string ownerProcess;
    int virtualPageNum = -1;
    std::vector<uint8_t> data;
};

class MemoryManager {
private:
    int totalMemory;
    int frameSize;
    int memPerProcess;
    int numFrames;
    int symbolTableSize = 64;
    
    std::vector<PhysicalFrame> physicalMemory;
    std::unordered_map<std::string, std::vector<PageTableEntry>> pageTables;
    std::unordered_map<std::string, std::vector<uint8_t>> backingStore;
    
    mutable std::mutex memoryMutex;
    std::string backingStoreFile = "csopesy-backing-store.txt";
    
    int pagesPagedIn = 0;
    int pagesPagedOut = 0;
    int totalMemoryAllocations = 0;
    int totalMemoryDeallocations = 0;
    
    int findFreeFrame();
    int selectVictimFrame();
    bool isFrameOccupied(int frameIndex) const;
    void evictFrame(int frameIndex);
    void loadPageToFrame(int virtualPageNum, const std::string& processName, int frameIndex);
    void savePageToBackingStore(const std::string& processName, int virtualPageNum);
    bool loadPageFromBackingStore(const std::string& processName, int virtualPageNum, int frameIndex);
    int calculateNumPages(int memorySize) const;
    bool isValidAddress(uint32_t address, std::shared_ptr<OSProcess> process) const;
    void initializeMemory();
    bool evictPage();
    int resolveFrameForPage(std::shared_ptr<OSProcess> process, uint32_t virtualAddress);
    
public:
    MemoryManager(int totalMem, int frameSize, int memPerProc);
    ~MemoryManager();

    bool allocateMemory(std::shared_ptr<OSProcess> process);
    void releaseMemory(std::shared_ptr<OSProcess> process);
    
    bool handlePageFault(std::shared_ptr<OSProcess> process, uint32_t virtualAddress);
    bool readMemory(uint32_t virtualAddress, uint16_t& value, std::shared_ptr<OSProcess> process);
    bool writeMemory(uint32_t virtualAddress, uint16_t value, std::shared_ptr<OSProcess> process);
    
    int getProcessCount() const;
    int calculateExternalFragmentation() const;
    std::string getMemoryMap() const;
    std::string getASCIIMemoryMap() const;
    
    void saveToBackingStore(const std::string& filename = "csopesy-backing-store.txt");
    void loadFromBackingStore(const std::string& filename = "csopesy-backing-store.txt");
    
    int getNumPagesPagedIn() const { return pagesPagedIn; }
    int getNumPagesPagedOut() const { return pagesPagedOut; }
    int getTotalMemoryAllocations() const { return totalMemoryAllocations; }
    int getTotalMemoryDeallocations() const { return totalMemoryDeallocations; }
    int getTotalMemory() const { return totalMemory; }
    int getUsedMemory() const;
    int getFrameSize() const { return frameSize; }
};