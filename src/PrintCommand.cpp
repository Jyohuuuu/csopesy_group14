#include "../include/PrintCommand.h"
#include "../include/ConsoleSync.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

std::mutex PrintCommand::fileMutex;