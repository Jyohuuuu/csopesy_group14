#pragma once
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <unordered_map>
#include <cstdint>
#include <stack>
#include <mutex>
#include "SymbolTable.h"
#include "ProcessInstructions.h"

class MemoryManager;

class OSProcess : public std::enable_shared_from_this<OSProcess> {
public:
    enum ProcessState {
        READY,
        RUNNING,
        WAITING,
        FINISHED
    };

    OSProcess(int pid, std::string name);
    
    void addInstruction(const Instruction& instruction);
    void executeNextInstruction(int coreId);
    bool isFinished() const;
    
    int getPID() const;
    ProcessState getState() const;
    std::string getName() const;
    SymbolTable& getSymbolTable();
    int getCommandCounter() const;
    int getTotalCommands() const;
    
    void setState(ProcessState state);
    
    void markStarted();
    void markEnded();
    bool hasStarted() const;
    bool hasEnded() const;
    std::string getStartTimeString() const;
    std::string getEndTimeString() const;
    
    void setWaitTicks(int ticks);
    void decrementWaitTicks();
    bool isWaiting() const;
    int getWaitTicks() const;

    bool hasMemory() const { return hasMemoryFlag; }
    void setHasMemory(bool v) { hasMemoryFlag = v; }

    int getMemorySize() const { return memorySize; }
    void setMemorySize(int size) { memorySize = size; }
    int getMemoryUsed() const { return memoryUsed; }
    void setMemoryUsed(int used) { memoryUsed = used; }
    
    bool hasMemoryViolation() const { return hasViolation; }
    uint32_t getViolationAddress() const { return violationAddress; }
    std::string getViolationTimeString() const;
    void setViolationAddress(uint32_t address);
    
    void setMemoryManager(std::shared_ptr<MemoryManager> mm) { memoryManager = mm; }
    
    std::vector<std::string> getOutputLogs() const {
        std::lock_guard<std::mutex> lock(outputLogsMutex);
        return outputLogs;
    }
    
    void addOutputLog(const std::string& log) {
        std::lock_guard<std::mutex> lock(outputLogsMutex);
        outputLogs.push_back(log);
    }
    
private:
    void executePrint(const Instruction& instr);
    void executeDeclare(const Instruction& instr);
    void executeAdd(const Instruction& instr);
    void executeSubtract(const Instruction& instr);
    void executeSleep(const Instruction& instr);
    void executeFor(const Instruction& instr);
    void executeRead(const Instruction& instr);
    void executeWrite(const Instruction& instr);
    
    uint16_t getVariableValue(const std::string& name);
    void setVariableValue(const std::string& name, uint16_t value);
    bool isVariable(const std::string& token);
    uint16_t parseValue(const std::string& token);
    uint32_t parseHexAddress(const std::string& str) const;
    std::string toHexString(uint32_t value) const;
    
    void logOutput(const std::string& msg) {
        std::lock_guard<std::mutex> lock(outputLogsMutex);
        outputLogs.push_back(msg);
    }
    
    int pid;
    std::string name;
    ProcessState currentState;
    std::vector<Instruction> instructionList;
    int instructionCounter;
    SymbolTable symbolTable;
    
    struct ForLoopState {
        int startIndex;
        int endIndex;
        int currentIteration;
        int maxIterations;
    };
    std::stack<ForLoopState> forLoopStack;
    bool insideForLoop;
    
    int waitTicks;
    bool isWaitingState;

    bool hasMemoryFlag = false;
    
    int memorySize = 4096;
    int memoryUsed = 0;
    
    bool hasViolation = false;
    uint32_t violationAddress = 0;
    std::chrono::time_point<std::chrono::system_clock> violationTime;
    
    std::shared_ptr<MemoryManager> memoryManager;
    
    std::chrono::time_point<std::chrono::system_clock> startTime;
    std::chrono::time_point<std::chrono::system_clock> endTime;
    bool started;
    bool ended;
    
    std::vector<std::string> outputLogs;
    mutable std::mutex outputLogsMutex;
};