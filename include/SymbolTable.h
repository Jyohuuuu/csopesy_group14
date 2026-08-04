#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>
#include <mutex>

class SymbolTable {
private:
    std::unordered_map<std::string, uint16_t> table;
    mutable std::mutex tableMutex;
    int maxVariables = 32;
    
public:
    SymbolTable() = default;
    
    void setValue(const std::string& name, uint16_t value) {
        std::lock_guard<std::mutex> lock(tableMutex);
        if (table.size() >= maxVariables && table.find(name) == table.end()) {
            return;
        }
        table[name] = value;
    }
    
    uint16_t getValue(const std::string& name) const {
        std::lock_guard<std::mutex> lock(tableMutex);
        auto it = table.find(name);
        if (it != table.end()) {
            return it->second;
        }
        return 0;
    }
    
    bool hasValue(const std::string& name) const {
        std::lock_guard<std::mutex> lock(tableMutex);
        return table.find(name) != table.end();
    }
    
    int getSize() const {
        std::lock_guard<std::mutex> lock(tableMutex);
        return static_cast<int>(table.size());
    }
    
    int getMaxVariables() const { return maxVariables; }
    
    void clear() {
        std::lock_guard<std::mutex> lock(tableMutex);
        table.clear();
    }
};