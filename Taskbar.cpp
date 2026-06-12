#include "Taskbar.h"
#include "TaskManager.h"
#include "imgui.h"
#include <cstdio>

// Global flags
static bool showAppWindow1 = false;
static bool showAppWindow2 = false;
static bool showTaskManager = false;

void RenderTaskbar()
{
    
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    
    // Taskbar at bottom
    float taskbarHeight = 60.0f;
    ImGui::SetNextWindowPos(ImVec2(0, screenSize.y - taskbarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(screenSize.x, taskbarHeight), ImGuiCond_Always);
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.15f, 0.95f));
    
    ImGui::Begin("##taskbar", nullptr, 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse);
    
    ImGui::PopStyleColor();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 10.0f));
    
    // Right-aligned buttons
    float buttonWidth = 80.0f;
    float buttonHeight = 40.0f;
    float rightEdge = screenSize.x - 20.0f;
    float startX = rightEdge - (buttonWidth * 3 + 20.0f * 2);
    
    ImGui::SetCursorPosX(startX);
    ImGui::SetCursorPosY(10.0f);
    
    // Button 1: App 1
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
    if (ImGui::Button("App 1", ImVec2(buttonWidth, buttonHeight)))
    {
        showAppWindow1 = !showAppWindow1;
    }
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
    
    // Button 2: App 2
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
    if (ImGui::Button("App 2", ImVec2(buttonWidth, buttonHeight)))
    {
        showAppWindow2 = !showAppWindow2;
    }
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
    
    // Button 3: Task Manager
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.5f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.6f, 0.3f, 1.0f));
    if (ImGui::Button("Task Mgr", ImVec2(buttonWidth, buttonHeight)))
    {
        showTaskManager = !showTaskManager;
    }
    ImGui::PopStyleColor(2);
    
    ImGui::PopStyleVar();
    ImGui::End();
    
    // App 1 Window
    if (showAppWindow1)
    {
        ImGui::Begin("Application 1 - Notes", &showAppWindow1);
        ImGui::SetWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        ImGui::Text("Welcome to Notes App!");
        ImGui::Separator();
        static char text[256] = "Write your notes here...";
        ImGui::InputTextMultiline("##notes", text, sizeof(text), ImVec2(380, 200));
        if (ImGui::Button("Save Note"))
        {
            ImGui::OpenPopup("Note Saved");
        }
        if (ImGui::BeginPopupModal("Note Saved"))
        {
            ImGui::Text("Your note has been saved!");
            if (ImGui::Button("OK"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        ImGui::End();
    }
    
    // App 2 Window
    if (showAppWindow2)
    {
        ImGui::Begin("Application 2 - System Info", &showAppWindow2);
        ImGui::SetWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        ImGui::Text("System Information");
        ImGui::Separator();
        ImGui::Text("OS: CSOPESY Mockup v1.0");
        ImGui::Text("CPU: Virtual Processor");
        ImGui::Text("RAM: 4 GB (Simulated)");
        ImGui::Text("Display: ImGui Renderer");
        ImGui::End();
    }
    
    // Task Manager (calls the separate function from TaskManager.cpp)
    if (showTaskManager)
    {
        RenderTaskManagerWindow(&showTaskManager);
    }
}
