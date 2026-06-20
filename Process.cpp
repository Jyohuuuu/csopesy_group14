#include "Process.h"
    #include <iostream>

Process::Process(int pid, std::string name) 
    : pid(pid), name(name), currentState(READY), commandCounter(0) {}

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