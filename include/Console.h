#pragma once
#include "Config.h"
#include "Scheduler.h"
#include <memory>
#include <string>
#include <unordered_map>

class Console {
public:
    Console();
    ~Console();
    void run();
    
private:
    bool initialized;
    std::unique_ptr<Scheduler> scheduler;
    bool inScreenSession;
    std::string currentScreenProcess;
    std::string currentScreenCommand;
    void clearScreen();
    void printMainMenu();
    void handleInitialize();
    void handleExit();
    void handleScreenLS();
    void handleScreenCreate(const std::string& name);
    void handleScreenCreate(const std::string& name, int memorySize);
    void handleScreenCustom(const std::string& name, int memorySize, const std::string& instructions);
    void handleScreenAttach(const std::string& name);
    void handleSchedulerStart();
    void handleSchedulerStop();
    void handleReportUtil();
    void handleVMStat();
    void processScreenCommand(const std::string& command);
    void runScreenSession(const std::string& name);
    
    std::vector<Instruction> parseCustomInstructions(const std::string& instructions);
    Instruction parseSingleInstruction(const std::string& instr, bool& valid);
    bool isValidMemorySize(int size) const;
};