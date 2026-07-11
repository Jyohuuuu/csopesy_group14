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

class OSProcess {
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
    
    uint16_t getVariableValue(const std::string& name);
    void setVariableValue(const std::string& name, uint16_t value);
    bool isVariable(const std::string& token);
    uint16_t parseValue(const std::string& token);
    
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
    
    std::chrono::time_point<std::chrono::system_clock> startTime;
    std::chrono::time_point<std::chrono::system_clock> endTime;
    bool started;
    bool ended;
    
    std::vector<std::string> outputLogs;
    mutable std::mutex outputLogsMutex;
};