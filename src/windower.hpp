#ifndef WINDOWER_H
#define WINDOWER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "renderer.hpp"

class Windower {
private:
    GLFWwindow* m_window;
    Renderer& m_renderer;

    int m_windowWidth;
    int m_windowHeight;

public:
    // Temporary input buffers
    char m_startNodeBuffer[32] = "0";
    char m_endNodeBuffer[32] = "0";

    std::vector<int64_t> m_finalPath;
    // ImGui INPUT VARIABLES (NEW)
    // ------------------------------

    // Mode 1: Node ID input
    int64_t m_startNode = 0;
    int64_t m_endNode = 0;

    bool m_runAStarWithNodes = false;

    // Mode 2: Coordinate input
    double m_startLat = 24.8600f;
    double m_startLon = 67.0100f;
    double m_endLat   = 24.8700f;
    double m_endLon   = 67.0200f;
    bool m_runAStarWithCoords = false;

    // Visual Settings
    float m_canvasScale = 1.0f;
    float m_pathColor[3] = {1.0f, 0.0f, 0.0f};

    // Search bar for road name
    char m_searchBuffer[128] = "";
    bool m_searchRequested = false;

    // --------------------------------
    bool m_middleDown;
    double m_lastMouseX;
    double m_lastMouseY;
    float m_camOX;
    float m_camOY;
    float m_camScale;

    static void m_framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void m_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void m_cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void m_scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    void processInput();
    void resizeViewport(GLFWwindow* window, int width, int height);


    Windower(Renderer& renderer, int windowWidth, int windowHeight);
    void run();
    ~Windower();
};

#endif
