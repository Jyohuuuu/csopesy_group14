#include "TaskManager.h"
#include "imgui.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>

struct Process {
    std::string name;
    int pid;
    float cpu;
    int memory;
    std::string status;
};

static std::vector<Process> processes = {
    {"System Idle Process", 0, 95.0f, 8, "Running"},
    {"csopesy_component1.exe", 1234, 2.5f, 45800, "Running"},
    {"explorer.exe", 5678, 1.2f, 89200, "Running"},
    {"chrome.exe", 9012, 8.7f, 245600, "Running"},
    {"notepad.exe", 3456, 0.3f, 12400, "Running"},
    {"svchost.exe", 1111, 0.5f, 45600, "Running"},
    {"dwm.exe", 2222, 1.1f, 67800, "Running"}
};

static int selectedTab = 0;
static float cpuHistory[60] = {0};
static float memoryHistory[60] = {0};
static int historyIndex = 0;

static void UpdatePerformanceHistory(float currentCpu, float currentMemory)
{
    cpuHistory[historyIndex] = currentCpu;
    memoryHistory[historyIndex] = currentMemory;
    historyIndex = (historyIndex + 1) % 60;
}

void RenderTaskManagerWindow(bool* p_open)
{
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    
    if (!ImGui::Begin("Task Manager", p_open))
    {
        ImGui::End();
        return;
    }
    
    // Menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                if (p_open) *p_open = false;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options"))
        {
            ImGui::MenuItem("Always on top", nullptr, false);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    // Tab bar
    ImGui::BeginChild("TabBar", ImVec2(0, 35), true);
    if (ImGui::Button("Processes", ImVec2(100, 25))) selectedTab = 0;
    ImGui::SameLine();
    if (ImGui::Button("Performance", ImVec2(100, 25))) selectedTab = 1;
    ImGui::SameLine();
    if (ImGui::Button("App history", ImVec2(100, 25))) selectedTab = 2;
    ImGui::EndChild();
    ImGui::Separator();
    
    // Processes Tab
    if (selectedTab == 0)
    {
        ImGui::BeginChild("ProcessList", ImVec2(0, 0), true);
        ImGui::TextUnformatted("Name");
        ImGui::SameLine(200);
        ImGui::TextUnformatted("PID");
        ImGui::SameLine(280);
        ImGui::TextUnformatted("CPU");
        ImGui::SameLine(360);
        ImGui::TextUnformatted("Memory");
        ImGui::Separator();
        
        for (const auto& proc : processes)
        {
            ImGui::TextUnformatted(proc.name.c_str());
            ImGui::SameLine(200);
            char pidStr[32];
            sprintf(pidStr, "%d", proc.pid);
            ImGui::TextUnformatted(pidStr);
            ImGui::SameLine(280);
            char cpuStr[32];
            sprintf(cpuStr, "%.1f%%", proc.cpu);
            ImGui::TextUnformatted(cpuStr);
            ImGui::SameLine(360);
            char memStr[32];
            sprintf(memStr, "%d MB", proc.memory / 1024);
            ImGui::TextUnformatted(memStr);
        }
        ImGui::EndChild();
    }
    
    // Performance Tab
    else if (selectedTab == 1)
    {
        static float cpuUsage = 45.0f;
        char cpuPercent[32];
        sprintf(cpuPercent, "%.0f%%", cpuUsage);
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "CPU");
        ImGui::ProgressBar(cpuUsage / 100.0f, ImVec2(-1, 25), cpuPercent);
        
        ImGui::TextUnformatted("CPU History:");
        ImGui::PlotLines("##cpuplot", cpuHistory, 60, 0, NULL, 0.0f, 100.0f, ImVec2(400, 80));
        
        static float memoryUsage = 8.4f;
        float memoryPercent = (memoryUsage / 16.0f) * 100.0f;
        char memProgress[64];
        sprintf(memProgress, "%.1f GB / 16.0 GB (%.0f%%)", memoryUsage, memoryPercent);
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.3f, 1.0f), "Memory");
        ImGui::ProgressBar(memoryPercent / 100.0f, ImVec2(-1, 25), memProgress);
        
        ImGui::TextUnformatted("Memory History:");
        ImGui::PlotLines("##memplot", memoryHistory, 60, 0, NULL, 0.0f, 100.0f, ImVec2(400, 80));
    }
    
    // App History Tab
    else if (selectedTab == 2)
    {
        ImGui::BeginChild("AppHistory", ImVec2(0, 0), true);
        ImGui::TextUnformatted("Name");
        ImGui::SameLine(250);
        ImGui::TextUnformatted("CPU Time");
        ImGui::Separator();
        ImGui::TextUnformatted("csopesy_component1.exe");
        ImGui::SameLine(250);
        ImGui::TextUnformatted("0:02:15");
        ImGui::EndChild();
    }
    
    // Update performance history
    static float timer = 0;
    timer += ImGui::GetIO().DeltaTime;
    if (timer >= 1.0f)
    {
        timer = 0;
        float cpu = 30.0f + (rand() % 70);
        float mem = 8.0f + (rand() % 2000) / 1000.0f;
        UpdatePerformanceHistory(cpu, mem);
    }
    
    ImGui::End();
}
