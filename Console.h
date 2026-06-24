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
    
    void printMainMenu();
    void handleInitialize();
    void handleExit();
    void handleScreenLS();
    void handleScreenCreate(const std::string& name);
    void handleScreenAttach(const std::string& name);
    void handleSchedulerStart();
    void handleSchedulerStop();
    void handleReportUtil();
    void processScreenCommand(const std::string& command);
    void runScreenSession(const std::string& name);
};