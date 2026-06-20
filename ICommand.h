#pragma once
#include <string>

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute(int coreId, int pid, const std::string& processName) = 0;
};