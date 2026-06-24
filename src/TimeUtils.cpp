#include "../include/TimeUtils.h"
#include <ctime>
#include <iomanip>
#include <sstream>

std::string GetCurrentTime()
{
    std::time_t now = std::time(nullptr);
    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%H:%M:%S");
    return oss.str();
}