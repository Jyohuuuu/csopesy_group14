#include "Desktop.h"
#include "Taskbar.h"
#include "TimeUtils.h"
#include "imgui.h"

static void DrawGradientBackground(ImVec2 screenSize)
{
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    ImU32 topColor    = IM_COL32(20, 24, 40, 255);
    ImU32 bottomColor = IM_COL32(10, 10, 15, 255);
    draw->AddRectFilledMultiColor(ImVec2(0, 0), screenSize, topColor, topColor, bottomColor, bottomColor);
}

void RenderDesktop(bool& running)
{
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    DrawGradientBackground(screenSize);
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    
    // Clock
    std::string timeStr = GetCurrentTime();
    ImVec2 textSize = ImGui::CalcTextSize(timeStr.c_str());
    draw->AddText(ImVec2(screenSize.x - textSize.x - 20, 20), IM_COL32(255, 255, 255, 255), timeStr.c_str());
    
    // Power button
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("##power_button", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("PWR")) { running = false; }
    ImGui::PopStyleColor(1);
    ImGui::End();
    
    // Taskbar
    RenderTaskbar();
}
