#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <random>
#include <iostream>
#include <iomanip>
#include <sstream>

enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR,
    READ,
    WRITE
};

struct Instruction {
    InstructionType type;
    std::vector<std::string> params;
    std::vector<Instruction> nestedInstructions;
    int repeatCount = 0;
    
    Instruction() = default;
    Instruction(InstructionType t, const std::vector<std::string>& p = {}) 
        : type(t), params(p) {}
};

class ProcessGenerator {
public:
    static std::vector<Instruction> generateInstructions(int minCount, int maxCount, 
                                                       const std::string& processName,
                                                       uint32_t memorySize) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> typeDist(0, 7);
        static std::uniform_int_distribution<> varDist(0, 9);
        static std::uniform_int_distribution<> valueDist(1, 100);
        static std::uniform_int_distribution<> sleepDist(1, 10);
        static std::uniform_int_distribution<> forNestedDist(1, 3);
        static std::uniform_int_distribution<> countDist(minCount, maxCount);
        uint32_t maxAlignedAddr = (memorySize >= 16) ? (memorySize / 16 - 1) : 0;
        std::uniform_int_distribution<> addressDist(0, static_cast<int>(maxAlignedAddr));
        int instructionCount = countDist(gen);
        std::vector<Instruction> instructions;
        
        for (int i = 0; i < instructionCount; ++i) {
            Instruction inst;
            int type = typeDist(gen);
            
            switch(type) {
                case 0: { // PRINT
                    inst.type = InstructionType::PRINT;
                    inst.params.push_back("Hello world from " + processName + "!");
                    break;
                }
                    
                case 1: { // DECLARE
                    inst.type = InstructionType::DECLARE;
                    inst.params.push_back("var" + std::to_string(varDist(gen)));
                    inst.params.push_back(std::to_string(valueDist(gen) % 100));
                    break;
                }
                    
                case 2: { // ADD
                    inst.type = InstructionType::ADD;
                    inst.params.push_back("var" + std::to_string(varDist(gen)));
                    inst.params.push_back("var" + std::to_string(varDist(gen)));
                    inst.params.push_back(std::to_string(valueDist(gen) % 50));
                    break;
                }
                    
                case 3: { // SUBTRACT
                    inst.type = InstructionType::SUBTRACT;
                    inst.params.push_back("var" + std::to_string(varDist(gen)));
                    inst.params.push_back("var" + std::to_string(varDist(gen)));
                    inst.params.push_back(std::to_string(valueDist(gen) % 50));
                    break;
                }
                    
                case 4: { // SLEEP
                    inst.type = InstructionType::SLEEP;
                    inst.params.push_back(std::to_string(sleepDist(gen)));
                    break;
                }
                    
                case 5: { // FOR
                    inst.type = InstructionType::FOR;
                    inst.repeatCount = valueDist(gen) % 5 + 1;
                    
                    int nestedCount = forNestedDist(gen);
                    for (int j = 0; j < nestedCount; ++j) {
                        Instruction nested;
                        int nestedType = typeDist(gen) % 4;
                        switch(nestedType) {
                            case 0:
                                nested.type = InstructionType::PRINT;
                                nested.params.push_back("Loop " + std::to_string(j+1) + " from " + processName);
                                break;
                            case 1:
                                nested.type = InstructionType::DECLARE;
                                nested.params.push_back("loopVar" + std::to_string(j));
                                nested.params.push_back(std::to_string(valueDist(gen) % 50));
                                break;
                            case 2:
                                nested.type = InstructionType::ADD;
                                nested.params.push_back("loopVar" + std::to_string(j));
                                nested.params.push_back("loopVar" + std::to_string(j));
                                nested.params.push_back(std::to_string(valueDist(gen) % 20));
                                break;
                            case 3:
                                nested.type = InstructionType::SUBTRACT;
                                nested.params.push_back("loopVar" + std::to_string(j));
                                nested.params.push_back("loopVar" + std::to_string(j));
                                nested.params.push_back(std::to_string(valueDist(gen) % 20));
                                break;
                        }
                        inst.nestedInstructions.push_back(nested);
                    }
                    break;
                }
                    
                case 6: { // READ
                    inst.type = InstructionType::READ;
                    std::string varName = "var" + std::to_string(varDist(gen));
                    uint32_t addr = addressDist(gen) * 16;
                    std::stringstream ss;
                    ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << addr;
                    inst.params = {varName, ss.str()};
                    break;
                }
                    
                case 7: { // WRITE
                    inst.type = InstructionType::WRITE;
                    uint32_t addr = addressDist(gen) * 16;
                    std::stringstream ss;
                    ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << addr;
                    int val = valueDist(gen) % 65535;
                    inst.params = {ss.str(), std::to_string(val)};
                    break;
                }
            }
            instructions.push_back(inst);
        }
        return instructions;
    }
};