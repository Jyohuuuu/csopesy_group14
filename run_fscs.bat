@echo off
title OS Emulator - FCFS Scheduler
echo ========================================
echo   OS Emulator with FCFS Scheduler
echo ========================================
echo.

REM Check if g++ is available
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: g++ compiler not found!
    echo Please install MinGW or add it to your PATH.
    pause
    exit /b 1
)

echo Compiling OS Emulator...
echo.

REM Compile with C++11 standard and pthread
g++ -std=c++11 -pthread -Wall -Wextra -O2 main_fscs.cpp Scheduler.cpp Process.cpp -o os_emulator.exe

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Compilation failed!
    echo Please check your source files.
    pause
    exit /b 1
)

echo Compilation successful!
echo.
echo Running OS Emulator...
echo ========================================
echo.

REM Run the emulator
os_emulator.exe

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Program crashed or exited with error!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Program finished successfully!
echo.

REM Check for generated text files
echo Generated process files:
dir /b process_*.txt 2>nul
if %errorlevel% neq 0 (
    echo No process text files found.
) else (
    echo.
    echo Total files: 
    dir /b process_*.txt 2>nul | find /c /v "" 
)

echo.
pause