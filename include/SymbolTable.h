#pragma once
#include <unordered_map>
#include <string>

class SymbolTable {
public:
    void setValue(const std::string& key, int value) {
        table[key] = value;
    }
    
    int getValue(const std::string& key) const {
        auto it = table.find(key);
        if (it != table.end()) {
            return it->second;
        }
        return 0;
    }
    
private:
    std::unordered_map<std::string, int> table;
};