@echo off
echo ========================================
echo   OS Emulator - Build Script
echo ========================================
echo.

REM Check for g++
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: g++ not found! Please install MinGW.
    echo Download from: https://www.mingw-w64.org/
    pause
    exit /b 1
)

REM Create config.txt if not exists
if not exist config.txt (
    echo Creating default config.txt...
    (
        echo num-cpu 4
        echo scheduler "rr"
        echo quantum-cycles 5
        echo batch-process-freq 1
        echo min-ins 1000
        echo max-ins 2000
        echo delay-per-exec 0
    ) > config.txt
    echo config.txt created.
)

echo.
echo Compiling...
echo.

REM Compile all needed source files
g++ -std=c++17 -Wall -Wextra -O2 ^
    main_mco1.cpp ^
    Console.cpp ^
    Scheduler.cpp ^
    Process.cpp ^
    PrintCommand.cpp ^
    -o OSEmulator.exe ^
    -lpthread

if %errorlevel% neq 0 (
    echo.
    echo ========================================
    echo   ERROR: Compilation failed!
    echo ========================================
    echo.
    echo Required source files:
    echo   - main_mco1.cpp
    echo   - Console.cpp
    echo   - Scheduler.cpp
    echo   - Process.cpp
    echo   - PrintCommand.cpp
    echo.
    echo Required headers:
    echo   - Console.h, Scheduler.h, Process.h
    echo   - PrintCommand.h, ICommand.h, Config.h
    echo   - FileUtils.h, SymbolTable.h, ConsoleSync.h
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Compilation Successful!
echo ========================================
echo.

REM Create required folders before running
if not exist process_logs (
    echo Creating process_logs folder...
    mkdir process_logs
)
if not exist reports (
    echo Creating reports folder...
    mkdir reports
)

echo.
echo Running OSEmulator.exe...
echo ========================================
echo.
OSEmulator.exe

echo.
echo ========================================
echo   Emulator finished
echo ========================================
pause