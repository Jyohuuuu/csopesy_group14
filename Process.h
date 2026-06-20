#pragma once
#include <memory>
#include <vector>
#include <string>
#include "ICommand.h"
#include "SymbolTable.h"

class Process {
public:
    enum ProcessState {
        READY,
        RUNNING,
        WAITING,
        FINISHED
    };

    Process(int pid, std::string name);
    void addCommand(std::shared_ptr<ICommand> command);
    void executeCurrentCommand(int coreId);
    void moveToNextLine();
    bool isFinished() const;
    int getPID() const;
    ProcessState getState() const;
    std::string getName() const;
    SymbolTable& getSymbolTable();
    void setState(ProcessState state);
    int getCommandCounter() const;

private:
    int pid;
    std::string name;
    ProcessState currentState;
    int commandCounter;
    std::vector<std::shared_ptr<ICommand>> commandList;
    SymbolTable symbolTable;
};