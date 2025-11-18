#ifndef IMGUI_PANEL_HPP
#define IMGUI_PANEL_HPP

#include "imgui.h"
#include "windower.hpp"

inline void ShowRouteTracerPanel(Windower& win) {
    ImGui::Begin("🚀 Karachi Route Tracer");

    ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "Pathfinding Controls");
    ImGui::Separator();
    ImGui::Spacing();

    static int mode = 0;  // 0 = Node IDs, 1 = Coordinates
    ImGui::Text("Select Pathfinding Mode:");
    ImGui::RadioButton("Node IDs", &mode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Coordinates", &mode, 1);
    ImGui::Spacing();

    if (mode == 0) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "📍 Node Search");
        ImGui::InputInt("Start Node", &win.m_startNode);
        ImGui::InputInt("End Node", &win.m_endNode);
        if (ImGui::Button("Run A* (Node IDs)")) win.m_runAStarWithNodes = true;
        ImGui::Spacing();
    }

    if (mode == 1) {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "📌 Coordinate Search");

        ImGui::Text("Start Location:");
        ImGui::InputFloat("Start Latitude", &win.m_startLat, 0.0f, 0.0f, "%.6f");
        ImGui::InputFloat("Start Longitude", &win.m_startLon, 0.0f, 0.0f, "%.6f");
        ImGui::Spacing();

        ImGui::Text("End Location:");
        ImGui::InputFloat("End Latitude", &win.m_endLat, 0.0f, 0.0f, "%.6f");
        ImGui::InputFloat("End Longitude", &win.m_endLon, 0.0f, 0.0f, "%.6f");

        if (ImGui::Button("Run A* (Coordinates)")) win.m_runAStarWithCoords = true;
        ImGui::Spacing();
    }

    ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.2f, 1.0f), "🔍 Search Road");
    ImGui::InputText("Road Name", win.m_searchBuffer, IM_ARRAYSIZE(win.m_searchBuffer));
    if (ImGui::Button("Search")) win.m_searchRequested = true;

    ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "🎨 Path Color");
    ImGui::ColorEdit3("Path Color", win.m_pathColor);

    ImGui::End();
}

#endif
