@echo off
setlocal enabledelayedexpansion

echo ========================================
echo   OS Emulator v2.0 - Build Script
echo ========================================
echo.

REM Check for g++
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: g++ not found! Please install MinGW.
    pause
    exit /b 1
)

REM Create config.txt if not exists
if not exist config.txt (
    echo Creating default config.txt...
    (
        echo num-cpu 2
        echo scheduler "rr"
        echo quantum-cycles 4
        echo batch-process-freq 1
        echo min-ins 100
        echo max-ins 100
        echo delay-per-exec 0
        echo max-overall-mem 16384
        echo mem-per-frame 16
        echo mem-per-proc 4096
    ) > config.txt
)

echo Compiling...
echo.

REM Clean old object files (optional)
if exist *.o del *.o

REM Compile with all source files
g++ -std=c++17 -Wall -Wextra -O2 -D_GNU_SOURCE ^
    src/main_mco1.cpp ^
    src/Console.cpp ^
    src/Scheduler.cpp ^
    src/Process.cpp ^
    src/PrintCommand.cpp ^
    src/MemoryManager.cpp ^
    src/SymbolTable.cpp ^
    -Iinclude ^
    -o OSEmulator.exe ^
    -pthread 2> build_errors.txt

if %errorlevel% neq 0 (
    echo.
    echo ========================================
    echo   ERROR: Compilation failed!
    echo ========================================
    exit /b 1
)

echo.
echo ========================================
echo   Compilation Successful!
echo ========================================
echo.

REM Create necessary directories
if not exist process_logs mkdir process_logs
if not exist reports mkdir reports
if not exist memory_stamps mkdir memory_stamps

echo Build complete. Run OSEmulator.exe manually when ready.
echo.
echo New Features Available:
echo   - Demand paging memory manager
echo   - READ/WRITE instructions
echo   - Custom instructions via screen -c
echo   - vmstat command for memory statistics
echo   - Backing store (csopesy-backing-store.txt)
echo.
echo Memory Configuration:
echo   - Total Memory: 16384 bytes
echo   - Frame Size: 16 bytes
echo   - Memory per Process: 4096 bytes
echo   - Max Processes in Memory: 4
echo.
pause