#include "../include/OSProcess.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <climits>

OSProcess::OSProcess(int pid, std::string name) 
    : pid(pid), name(name), currentState(READY), instructionCounter(0),
      insideForLoop(false), waitTicks(0), isWaitingState(false),
      started(false), ended(false) {}

void OSProcess::addInstruction(const Instruction& instruction) {
    instructionList.push_back(instruction);
}

void OSProcess::executeNextInstruction(int coreId) {
    if (isWaitingState) {
        decrementWaitTicks();
        if (waitTicks <= 0) {
            isWaitingState = false;
            currentState = RUNNING;
        }
        return;
    }
    
    if (instructionCounter >= static_cast<int>(instructionList.size())) {
        currentState = FINISHED;
        return;
    }
    
    const Instruction& instr = instructionList[instructionCounter];
    
    if (insideForLoop) {
        ForLoopState& state = forLoopStack.top();
        if (state.currentIteration < state.maxIterations) {
            if (instructionCounter < state.endIndex) {
                const Instruction& nestedInstr = instructionList[instructionCounter];
                switch(nestedInstr.type) {
                    case InstructionType::PRINT: executePrint(nestedInstr); break;
                    case InstructionType::DECLARE: executeDeclare(nestedInstr); break;
                    case InstructionType::ADD: executeAdd(nestedInstr); break;
                    case InstructionType::SUBTRACT: executeSubtract(nestedInstr); break;
                    default: break;
                }
                instructionCounter++;
            } else {
                state.currentIteration++;
                if (state.currentIteration < state.maxIterations) {
                    instructionCounter = state.startIndex;
                } else {
                    forLoopStack.pop();
                    insideForLoop = !forLoopStack.empty();
                    instructionCounter = state.endIndex + 1;
                }
            }
            return;
        } else {
            forLoopStack.pop();
            insideForLoop = !forLoopStack.empty();
            instructionCounter = state.endIndex + 1;
            return;
        }
    }
    
    switch(instr.type) {
        case InstructionType::PRINT:
            executePrint(instr);
            instructionCounter++;
            break;
        case InstructionType::DECLARE:
            executeDeclare(instr);
            instructionCounter++;
            break;
        case InstructionType::ADD:
            executeAdd(instr);
            instructionCounter++;
            break;
        case InstructionType::SUBTRACT:
            executeSubtract(instr);
            instructionCounter++;
            break;
        case InstructionType::SLEEP:
            executeSleep(instr);
            instructionCounter++;
            break;
        case InstructionType::FOR:
            executeFor(instr);
            break;
        default:
            instructionCounter++;
            break;
    }
}

void OSProcess::executePrint(const Instruction& instr) {
    if (instr.params.empty()) return;
    
    std::string msg = instr.params[0];
    
    size_t varPos = msg.find('+');
    if (varPos != std::string::npos) {
        std::string varName = msg.substr(varPos + 1);
        varName.erase(std::remove_if(varName.begin(), varName.end(), ::isspace), varName.end());
        uint16_t value = getVariableValue(varName);
        std::string prefix = msg.substr(0, varPos);
        while (!prefix.empty() && prefix.back() == ' ') prefix.pop_back();
        msg = prefix + ": " + std::to_string(value);
    } else {
        std::string trimmed = msg;
        trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
        if (isVariable(trimmed)) {
            uint16_t value = getVariableValue(trimmed);
            msg = trimmed + " = " + std::to_string(value);
        }
    }
    
    outputLogs.push_back("[PRINT] " + msg);
}

void OSProcess::executeDeclare(const Instruction& instr) {
    if (instr.params.size() < 2) return;
    std::string varName = instr.params[0];
    uint16_t value = parseValue(instr.params[1]);
    symbolTable.setValue(varName, value);
    
    outputLogs.push_back("[DECLARE] " + varName + " = " + std::to_string(value));
}

void OSProcess::executeAdd(const Instruction& instr) {
    if (instr.params.size() < 3) return;
    std::string destVar = instr.params[0];
    uint16_t val1 = parseValue(instr.params[1]);
    uint16_t val2 = parseValue(instr.params[2]);
    uint32_t result = static_cast<uint32_t>(val1) + static_cast<uint32_t>(val2);
    if (result > UINT16_MAX) result = UINT16_MAX;
    setVariableValue(destVar, static_cast<uint16_t>(result));
    
    outputLogs.push_back("[ADD] " + destVar + " = " + std::to_string(val1) + " + " + std::to_string(val2) + " = " + std::to_string(result));
}

void OSProcess::executeSubtract(const Instruction& instr) {
    if (instr.params.size() < 3) return;
    std::string destVar = instr.params[0];
    uint16_t val1 = parseValue(instr.params[1]);
    uint16_t val2 = parseValue(instr.params[2]);
    int32_t result = static_cast<int32_t>(val1) - static_cast<int32_t>(val2);
    if (result < 0) result = 0;
    if (result > UINT16_MAX) result = UINT16_MAX;
    setVariableValue(destVar, static_cast<uint16_t>(result));
    
    outputLogs.push_back("[SUBTRACT] " + destVar + " = " + std::to_string(val1) + " - " + std::to_string(val2) + " = " + std::to_string(result));
}

void OSProcess::executeSleep(const Instruction& instr) {
    if (instr.params.empty()) return;
    int ticks = parseValue(instr.params[0]);
    if (ticks < 1) ticks = 1;
    if (ticks > 255) ticks = 255;
    setWaitTicks(ticks);
    currentState = WAITING;
    
    outputLogs.push_back("[SLEEP] Sleeping for " + std::to_string(ticks) + " ticks");
}

void OSProcess::executeFor(const Instruction& instr) {
    if (instr.nestedInstructions.empty()) {
        instructionCounter++;
        return;
    }
    
    int startIndex = instructionCounter + 1;
    for (const auto& nested : instr.nestedInstructions) {
        instructionList.insert(instructionList.begin() + instructionCounter + 1, nested);
        instructionCounter++;
    }
    int endIndex = instructionCounter;
    
    ForLoopState state;
    state.startIndex = startIndex;
    state.endIndex = endIndex;
    state.currentIteration = 0;
    state.maxIterations = instr.repeatCount;
    forLoopStack.push(state);
    insideForLoop = true;
    instructionCounter = startIndex;
    
    outputLogs.push_back("[FOR] Starting loop with " + std::to_string(instr.repeatCount) + " iterations");
}

uint16_t OSProcess::getVariableValue(const std::string& name) {
    if (!symbolTable.hasValue(name)) {
        symbolTable.setValue(name, 0);
    }
    return static_cast<uint16_t>(symbolTable.getValue(name));
}

void OSProcess::setVariableValue(const std::string& name, uint16_t value) {
    symbolTable.setValue(name, value);
}

bool OSProcess::isVariable(const std::string& token) {
    if (token.empty()) return false;
    return (token[0] == 'v' || token[0] == 'V' || token[0] == 'l') && 
           std::all_of(token.begin(), token.end(), [](char c) { return std::isalnum(c); });
}

uint16_t OSProcess::parseValue(const std::string& token) {
    if (isVariable(token)) {
        return getVariableValue(token);
    }
    try {
        int val = std::stoi(token);
        if (val < 0) return 0;
        if (val > UINT16_MAX) return UINT16_MAX;
        return static_cast<uint16_t>(val);
    } catch (...) {
        return 0;
    }
}

bool OSProcess::isFinished() const { return currentState == FINISHED; }
int OSProcess::getPID() const { return pid; }
OSProcess::ProcessState OSProcess::getState() const { return currentState; }
std::string OSProcess::getName() const { return name; }
SymbolTable& OSProcess::getSymbolTable() { return symbolTable; }
void OSProcess::setState(ProcessState state) { currentState = state; }
int OSProcess::getCommandCounter() const { return instructionCounter; }
int OSProcess::getTotalCommands() const { return static_cast<int>(instructionList.size()); }

void OSProcess::markStarted() { startTime = std::chrono::system_clock::now(); started = true; }
void OSProcess::markEnded() { endTime = std::chrono::system_clock::now(); ended = true; }
bool OSProcess::hasStarted() const { return started; }
bool OSProcess::hasEnded() const { return ended; }

std::string OSProcess::getStartTimeString() const {
    if (!started) return "N/A";
    auto time_t = std::chrono::system_clock::to_time_t(startTime);
    std::tm tm = *std::localtime(&time_t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%m/%d/%Y %I:%M:%S");
    ss << (tm.tm_hour >= 12 ? "PM" : "AM");
    return ss.str();
}

std::string OSProcess::getEndTimeString() const {
    if (!ended) return "N/A";
    auto time_t = std::chrono::system_clock::to_time_t(endTime);
    std::tm tm = *std::localtime(&time_t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%m/%d/%Y %I:%M:%S");
    ss << (tm.tm_hour >= 12 ? "PM" : "AM");
    return ss.str();
}

void OSProcess::setWaitTicks(int ticks) { waitTicks = ticks; isWaitingState = true; }
void OSProcess::decrementWaitTicks() { if (waitTicks > 0) waitTicks--; }
bool OSProcess::isWaiting() const { return isWaitingState; }
int OSProcess::getWaitTicks() const { return waitTicks; }