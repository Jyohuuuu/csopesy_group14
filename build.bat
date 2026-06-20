@echo off
g++ -std=c++17 -g -ID:/glfw-3.4.bin.WIN64/include -LD:/glfw-3.4.bin.WIN64/lib-mingw-w64 -Iimgui -Iimgui/backends main.cpp Desktop.cpp Taskbar.cpp TaskManager.cpp TimeUtils.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp -o csopesy_component1.exe -lglfw3 -lopengl32 -lgdi32 -Wno-nullability-completeness

if %errorlevel% == 0 (
    echo Build successful! Run csopesy_component1.exe
) else (
    echo Build failed!
)