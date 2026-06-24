#include "../include/OSProcess.h"
#include <iostream>

Process::Process(int pid, std::string name) 
    : pid(pid), name(name), currentState(READY), commandCounter(0),
      started(false), ended(false) {}

void Process::addCommand(std::shared_ptr<ICommand> command) {
    commandList.push_back(command);
}

void Process::executeCurrentCommand(int coreId) {
    if (commandCounter < static_cast<int>(commandList.size())) {
        commandList[commandCounter]->execute(coreId, pid, name);
    }
}

void Process::moveToNextLine() {
    commandCounter++;
    if (commandCounter >= static_cast<int>(commandList.size())) {
        currentState = FINISHED;
    }
}

bool Process::isFinished() const {
    return currentState == FINISHED;
}

int Process::getPID() const {
    return pid;
}

Process::ProcessState Process::getState() const {
    return currentState;
}

std::string Process::getName() const {
    return name;
}

SymbolTable& Process::getSymbolTable() {
    return symbolTable;
}

void Process::setState(ProcessState state) {
    currentState = state;
}

int Process::getCommandCounter() const {
    return commandCounter;
}

int Process::getTotalCommands() const {
    return static_cast<int>(commandList.size());
}

// New timestamp implementations
void Process::markStarted() {
    startTime = std::chrono::system_clock::now();
    started = true;
}

void Process::markEnded() {
    endTime = std::chrono::system_clock::now();
    ended = true;
}

bool Process::hasStarted() const {
    return started;
}

bool Process::hasEnded() const {
    return ended;
}

std::string Process::getStartTimeString() const {
    if (!started) return "N/A";
    
    auto time_t = std::chrono::system_clock::to_time_t(startTime);
    std::tm tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%m/%d/%Y %I:%M:%S");
    
    // Add AM/PM
    if (tm.tm_hour >= 12) {
        ss << "PM";
    } else {
        ss << "AM";
    }
    return ss.str();
}

std::string Process::getEndTimeString() const {
    if (!ended) return "N/A";
    
    auto time_t = std::chrono::system_clock::to_time_t(endTime);
    std::tm tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%m/%d/%Y %I:%M:%S");
    
    // Add AM/PM
    if (tm.tm_hour >= 12) {
        ss << "PM";
    } else {
        ss << "AM";
    }
    return ss.str();
}