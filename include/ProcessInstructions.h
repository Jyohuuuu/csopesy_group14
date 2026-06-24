#pragma once
#include <string>
#include <vector>
#include <functional>
#include <random>

enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR
};

struct Instruction {
    InstructionType type;
    std::vector<std::string> params;
    std::vector<Instruction> nestedInstructions; // For FOR loops
    int repeatCount = 0;
};

class ProcessGenerator {
public:
    static std::vector<Instruction> generateInstructions(int minCount, int maxCount) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> countDist(minCount, maxCount);
        static std::uniform_int_distribution<> typeDist(0, 5);
        static std::uniform_int_distribution<> varDist(0, 9);
        static std::uniform_int_distribution<> valueDist(1, 100);
        static std::uniform_int_distribution<> sleepDist(1, 10);
        
        int instructionCount = countDist(gen);
        std::vector<Instruction> instructions;
        
        for (int i = 0; i < instructionCount; ++i) {
            Instruction inst;
            int type = typeDist(gen);
            
            switch(type) {
                case 0: // PRINT
                    inst.type = InstructionType::PRINT;
                    inst.params.push_back("Hello world from " + getProcessName());
                    break;
                case 1: // DECLARE
                    inst.type = InstructionType::DECLARE;
                    inst.params.push_back("var" + std::to_string(varDist(gen)));
                    inst.params.push_back(std::to_string(valueDist(gen)));
                    break;
                case 2: // ADD
                    inst.type = InstructionType::ADD;
                    inst.params.push_back("var" + std::to_string(varDist(gen)));
                    inst.params.push_back("var" + std::to_string(varDist(gen)));
                    inst.params.push_back(std::to_string(valueDist(gen)));
                    break;
                case 3: // SUBTRACT
                    inst.type = InstructionType::SUBTRACT;
                    inst.params.push_back("var" + std::to_string(varDist(gen)));
                    inst.params.push_back("var" + std::to_string(varDist(gen)));
                    inst.params.push_back(std::to_string(valueDist(gen)));
                    break;
                case 4: // SLEEP
                    inst.type = InstructionType::SLEEP;
                    inst.params.push_back(std::to_string(sleepDist(gen)));
                    break;
                case 5: // FOR loop
                    inst.type = InstructionType::FOR;
                    inst.repeatCount = valueDist(gen) % 5 + 1;
                    int nestedCount = valueDist(gen) % 3 + 1;
                    for (int j = 0; j < nestedCount; ++j) {
                        Instruction nested;
                        nested.type = InstructionType::PRINT;
                        nested.params.push_back("Loop iteration " + std::to_string(j + 1));
                        inst.nestedInstructions.push_back(nested);
                    }
                    break;
            }
            instructions.push_back(inst);
        }
        return instructions;
    }
    
private:
    static std::string getProcessName() {
        static int counter = 0;
        return "p" + std::to_string(++counter);
    }
};