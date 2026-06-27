@echo off
setlocal enabledelayedexpansion

echo ========================================
echo   OS Emulator - Build Script
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
        echo num-cpu 4
        echo scheduler "rr"
        echo quantum-cycles 5
        echo batch-process-freq 1
        echo min-ins 1000
        echo max-ins 2000
        echo delay-per-exec 0
    ) > config.txt
)

echo Compiling...
echo.

REM Compile and capture ONLY errors (not warnings) to a file
g++ -std=c++17 -Wall -Wextra -O2 -D_GNU_SOURCE ^
    src/main_mco1.cpp ^
    src/Console.cpp ^
    src/Scheduler.cpp ^
    src/Process.cpp ^
    src/PrintCommand.cpp ^
    -Iinclude ^
    -o OSEmulator.exe ^
    -pthread 2> build_errors.txt

if %errorlevel% neq 0 (
    echo.
    echo ========================================
    echo   ERROR: Compilation failed!
    echo ========================================
    echo.
    echo Error log saved to: build_errors.txt
    echo.
    echo First 20 errors:
    echo ----------------------------------------
    type build_errors.txt | findstr /C:"error:" | head -n 20
    echo.
    echo To see full errors, open build_errors.txt
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Compilation Successful!
echo ========================================
echo.

if not exist process_logs mkdir process_logs
if not exist reports mkdir reports

echo Running OSEmulator.exe...
echo ========================================
echo.
OSEmulator.exe