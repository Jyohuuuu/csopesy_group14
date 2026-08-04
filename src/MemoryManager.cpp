#include "../include/MemoryManager.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <filesystem>

MemoryManager::MemoryManager(int totalMem, int frameSize, int memPerProc)
    : totalMemory(totalMem), frameSize(frameSize), memPerProcess(memPerProc) {
    numFrames = totalMemory / frameSize;
    physicalMemory.resize(numFrames);
    initializeMemory();
    loadFromBackingStore(backingStoreFile);
}

MemoryManager::~MemoryManager() {
    saveToBackingStore(backingStoreFile);
}

void MemoryManager::initializeMemory() {
    for (auto& frame : physicalMemory) {
        frame.occupied = false;
        frame.data.resize(frameSize, 0);
    }
}

int MemoryManager::getUsedMemory() const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    int usedMemory = 0;
    for (const auto& frame : physicalMemory) {
        if (frame.occupied) {
            usedMemory += frameSize;
        }
    }
    return usedMemory;
}

bool MemoryManager::allocateMemory(std::shared_ptr<OSProcess> process) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    if (!process) return false;
    
    int neededFrames = calculateNumPages(process->getMemorySize());
    
    int reservedFrames = 0;
    for (const auto& [name, table] : pageTables) {
        reservedFrames += static_cast<int>(table.size());
    }
    int freeFrames = numFrames - reservedFrames;
    
    if (freeFrames < neededFrames) {
        return false;
    }
    
    std::vector<PageTableEntry> pageTable;
    for (int i = 0; i < neededFrames; i++) {
        PageTableEntry entry;
        entry.valid = false;
        entry.frameIndex = -1;
        entry.virtualPageNum = i;
        entry.processName = process->getName();
        entry.referenced = false;
        entry.modified = false;
        pageTable.push_back(entry);
    }
    
    pageTables[process->getName()] = pageTable;
    totalMemoryAllocations++;
    return true;
}

void MemoryManager::releaseMemory(std::shared_ptr<OSProcess> process) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    if (!process) return;
    
    auto it = pageTables.find(process->getName());
    if (it != pageTables.end()) {
        for (const auto& entry : it->second) {
            if (entry.valid && entry.frameIndex >= 0 && entry.frameIndex < numFrames) {
                bool hasData = false;
                for (uint8_t byte : physicalMemory[entry.frameIndex].data) {
                    if (byte != 0) { hasData = true; break; }
                }
                
                if (hasData) {
                    savePageToBackingStore(process->getName(), entry.virtualPageNum);
                    pagesPagedOut++;
                }
                
                physicalMemory[entry.frameIndex].occupied = false;
                physicalMemory[entry.frameIndex].ownerProcess = "";
                physicalMemory[entry.frameIndex].virtualPageNum = -1;
                std::fill(physicalMemory[entry.frameIndex].data.begin(), 
                         physicalMemory[entry.frameIndex].data.end(), 0);
            }
        }
        pageTables.erase(it);
        totalMemoryDeallocations++;
    }
}

int MemoryManager::resolveFrameForPage(std::shared_ptr<OSProcess> process, uint32_t virtualAddress) {
    if (!process) return -1;

    int virtualPageNum = virtualAddress / frameSize;

    auto it = pageTables.find(process->getName());
    if (it == pageTables.end()) return -1;

    for (const auto& entry : it->second) {
        if (entry.virtualPageNum == virtualPageNum && entry.valid) {
            return entry.frameIndex;
        }
    }

    int freeFrame = findFreeFrame();
    if (freeFrame == -1) {
        if (!evictPage()) return -1;
        freeFrame = findFreeFrame();
        if (freeFrame == -1) return -1;
    }

    if (!loadPageFromBackingStore(process->getName(), virtualPageNum, freeFrame)) {
        physicalMemory[freeFrame].occupied = true;
        physicalMemory[freeFrame].ownerProcess = process->getName();
        physicalMemory[freeFrame].virtualPageNum = virtualPageNum;
        std::fill(physicalMemory[freeFrame].data.begin(),
                 physicalMemory[freeFrame].data.end(), 0);
        pagesPagedIn++;
    }

    for (auto& entry : it->second) {
        if (entry.virtualPageNum == virtualPageNum) {
            entry.valid = true;
            entry.frameIndex = freeFrame;
            entry.referenced = true;
            entry.modified = false;
            break;
        }
    }

    return freeFrame;
}

bool MemoryManager::handlePageFault(std::shared_ptr<OSProcess> process, uint32_t virtualAddress) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    return resolveFrameForPage(process, virtualAddress) != -1;
}

bool MemoryManager::readMemory(uint32_t virtualAddress, uint16_t& value, std::shared_ptr<OSProcess> process) {
    if (!process) return false;
    if (!isValidAddress(virtualAddress, process)) return false;

    std::lock_guard<std::mutex> lock(memoryMutex);

    int offset = virtualAddress % frameSize;
    int frameIndex = resolveFrameForPage(process, virtualAddress);
    if (frameIndex < 0 || frameIndex >= numFrames) return false;
    if (offset + 2 > frameSize) return false;

    value = physicalMemory[frameIndex].data[offset] |
            (physicalMemory[frameIndex].data[offset + 1] << 8);
    return true;
}

bool MemoryManager::writeMemory(uint32_t virtualAddress, uint16_t value, std::shared_ptr<OSProcess> process) {
    if (!process) return false;
    if (!isValidAddress(virtualAddress, process)) return false;

    std::lock_guard<std::mutex> lock(memoryMutex);

    int offset = virtualAddress % frameSize;
    int frameIndex = resolveFrameForPage(process, virtualAddress);
    if (frameIndex < 0 || frameIndex >= numFrames) return false;
    if (offset + 2 > frameSize) return false;

    physicalMemory[frameIndex].data[offset] = static_cast<uint8_t>(value & 0xFF);
    physicalMemory[frameIndex].data[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);

    auto it = pageTables.find(process->getName());
    if (it != pageTables.end()) {
        for (auto& entry : it->second) {
            if (entry.frameIndex == frameIndex) {
                entry.modified = true;
                break;
            }
        }
    }

    return true;
}
int MemoryManager::findFreeFrame() {
    for (int i = 0; i < numFrames; i++) {
        if (!physicalMemory[i].occupied) {
            return i;
        }
    }
    return -1;
}

bool MemoryManager::evictPage() {
    for (int i = 0; i < numFrames; i++) {
        if (physicalMemory[i].occupied) {
            std::string processName = physicalMemory[i].ownerProcess;
            int virtualPageNum = physicalMemory[i].virtualPageNum;
            
            bool hasData = false;
            for (uint8_t byte : physicalMemory[i].data) {
                if (byte != 0) {
                    hasData = true;
                    break;
                }
            }
            
            if (hasData) {
                savePageToBackingStore(processName, virtualPageNum);
                pagesPagedOut++;
            }
            
            physicalMemory[i].occupied = false;
            physicalMemory[i].ownerProcess = "";
            physicalMemory[i].virtualPageNum = -1;
            std::fill(physicalMemory[i].data.begin(), 
                     physicalMemory[i].data.end(), 0);
            
            auto it = pageTables.find(processName);
            if (it != pageTables.end()) {
                for (auto& entry : it->second) {
                    if (entry.frameIndex == i) {
                        entry.valid = false;
                        entry.frameIndex = -1;
                        break;
                    }
                }
            }
            
            return true;
        }
    }
    
    return false;
}

bool MemoryManager::isValidAddress(uint32_t address, std::shared_ptr<OSProcess> process) const {
    if (!process) return false;
    
    uint32_t processMemorySize = process->getMemorySize();
    if (address >= processMemorySize) {
        return false;
    }
    
    return true;
}

int MemoryManager::calculateNumPages(int memorySize) const {
    return (memorySize + frameSize - 1) / frameSize;
}

void MemoryManager::saveToBackingStore(const std::string& filename) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    for (const auto& [processName, pageTable] : pageTables) {
        for (const auto& entry : pageTable) {
            if (entry.valid && entry.frameIndex >= 0 && entry.frameIndex < numFrames) {
                const auto& frame = physicalMemory[entry.frameIndex];
                if (!frame.data.empty()) {
                    file << processName << "," << entry.virtualPageNum << ",";
                    for (size_t i = 0; i < frame.data.size(); i++) {
                        file << std::hex << std::setw(2) << std::setfill('0') 
                             << static_cast<int>(frame.data[i]);
                    }
                    file << "\n";
                }
            }
        }
    }
    
    file.close();
}

void MemoryManager::loadFromBackingStore(const std::string& filename) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    
    std::ifstream file(filename);
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string processName, pageNumStr, dataStr;
        
        std::getline(ss, processName, ',');
        std::getline(ss, pageNumStr, ',');
        std::getline(ss, dataStr);
        
        if (!processName.empty() && !pageNumStr.empty() && !dataStr.empty()) {
            try {
                int pageNum = std::stoi(pageNumStr);
                std::vector<uint8_t> data;
                
                for (size_t i = 0; i < dataStr.length(); i += 2) {
                    if (i + 2 <= dataStr.length()) {
                        std::string byteStr = dataStr.substr(i, 2);
                        int byteVal = std::stoi(byteStr, nullptr, 16);
                        data.push_back(static_cast<uint8_t>(byteVal));
                    }
                }
                
                if (!data.empty()) {
                    std::string key = processName + "_" + std::to_string(pageNum);
                    backingStore[key] = data;
                }
            } catch (...) {
            }
        }
    }
    
    file.close();
}

void MemoryManager::savePageToBackingStore(const std::string& processName, int virtualPageNum) {
    std::string key = processName + "_" + std::to_string(virtualPageNum);
    
    for (int i = 0; i < numFrames; i++) {
        if (physicalMemory[i].occupied && 
            physicalMemory[i].ownerProcess == processName &&
            physicalMemory[i].virtualPageNum == virtualPageNum) {
            backingStore[key] = physicalMemory[i].data;
            break;
        }
    }
}

bool MemoryManager::loadPageFromBackingStore(const std::string& processName, int virtualPageNum, int frameIndex) {
    std::string key = processName + "_" + std::to_string(virtualPageNum);
    
    auto it = backingStore.find(key);
    if (it == backingStore.end()) return false;
    
    if (frameIndex >= 0 && frameIndex < numFrames) {
        physicalMemory[frameIndex].data = it->second;
        physicalMemory[frameIndex].occupied = true;
        physicalMemory[frameIndex].ownerProcess = processName;
        physicalMemory[frameIndex].virtualPageNum = virtualPageNum;
        pagesPagedIn++;
        return true;
    }
    
    return false;
}

int MemoryManager::getProcessCount() const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    std::unordered_map<std::string, bool> uniqueProcesses;
    for (const auto& frame : physicalMemory) {
        if (frame.occupied && !frame.ownerProcess.empty()) {
            uniqueProcesses[frame.ownerProcess] = true;
        }
    }
    return uniqueProcesses.size();
}

int MemoryManager::calculateExternalFragmentation() const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    int freeMemory = 0;
    for (const auto& frame : physicalMemory) {
        if (!frame.occupied) {
            freeMemory += frameSize;
        }
    }
    return freeMemory;
}

std::string MemoryManager::getMemoryMap() const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    std::stringstream ss;
    
    ss << "Memory Map (Frame Size: " << frameSize << " bytes)\n";
    ss << "========================================\n";
    
    for (int i = 0; i < numFrames; i++) {
        const auto& frame = physicalMemory[i];
        if (frame.occupied) {
            ss << "Frame " << std::setw(3) << i << ": [USED] " 
               << frame.ownerProcess << " (Page " << frame.virtualPageNum << ")\n";
        } else {
            ss << "Frame " << std::setw(3) << i << ": [FREE]\n";
        }
    }
    
    return ss.str();
}

std::string MemoryManager::getASCIIMemoryMap() const {
    std::lock_guard<std::mutex> lock(memoryMutex);
    std::stringstream ss;
    
    ss << "Memory Map (Frame Size: " << frameSize << " bytes)\n";
    ss << "========================================\n";
    ss << "| Frame | Status | Owner | Page |\n";
    ss << "|-------|--------|-------|------|\n";
    
    for (int i = 0; i < numFrames; i++) {
        const auto& frame = physicalMemory[i];
        ss << "| " << std::setw(5) << i << " | ";
        if (frame.occupied) {
            ss << "USED    | " << std::setw(5) << frame.ownerProcess << " | " 
               << std::setw(4) << frame.virtualPageNum << " |\n";
        } else {
            ss << "FREE    |       |      |\n";
        }
    }
    
    return ss.str();
}