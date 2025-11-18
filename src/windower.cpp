#include <iostream>
#include <cmath>

#include "imgui_panel.hpp"
#include "windower.hpp"


// Modern Dark Theme Function
// --------------------------
void ApplyModernDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 6.0f;

    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;

    style.ItemSpacing = ImVec2(10, 10);
    style.WindowPadding = ImVec2(12, 12);

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.35f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.45f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.45f, 0.55f, 1.0f);

    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.30f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.40f, 0.50f, 1.0f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.28f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.32f, 1.0f);
}


Windower::Windower(Renderer& renderer, int windowWidth, int windowHeight)
    : m_renderer(renderer), m_windowWidth(windowWidth), m_windowHeight(windowHeight)
{
    if (!glfwInit()) {
        std::cout << "GLFW not initialized!" << std::endl;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Hello World", nullptr, nullptr);
    if (!m_window) {
        std::cout << "Failed to initialize GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    glViewport(0, 0, m_windowWidth, m_windowHeight);
    glfwSetFramebufferSizeCallback(m_window, m_framebufferSizeCallback);

    glfwSetCursorPosCallback(m_window, m_cursorPosCallback);
    glfwSetMouseButtonCallback(m_window, m_mouseButtonCallback);

    glfwSetScrollCallback(m_window, m_scrollCallback);
    glfwSetWindowUserPointer(m_window, this);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ApplyModernDarkTheme();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;     

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);          
    ImGui_ImplOpenGL3_Init();

    m_middleDown = false;
    m_lastMouseX = 0.0;
    m_lastMouseY = 0.0;
    m_camOX = 0.0f;
    m_camOY = 0.0f;
    m_camScale = 1.0f;

    m_renderer.defineGeometry();
    m_renderer.setCamera(m_camOX, m_camOY, m_camScale);
}

void Windower::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        processInput();        

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        ShowRouteTracerPanel(*this);


        m_renderer.render();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(m_window);
    }
}

void Windower::processInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }

    m_renderer.setCamera(m_camOX, m_camOY, m_camScale);
}


void Windower::m_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Windower* win = reinterpret_cast<Windower*>(glfwGetWindowUserPointer(window));
    if (!win) return;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            win->m_middleDown = true;
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            win->m_lastMouseX = x;
            win->m_lastMouseY = y;
        } else if (action == GLFW_RELEASE) {
            win->m_middleDown = false;
        }
    }
}

void Windower::m_cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    Windower* win = reinterpret_cast<Windower*>(glfwGetWindowUserPointer(window));
    if (!win) return;
    if (!win->m_middleDown) return;

    double dx = xpos - win->m_lastMouseX;
    double dy = ypos - win->m_lastMouseY;
    win->m_lastMouseX = xpos;
    win->m_lastMouseY = ypos;

    // convert pixel delta to NDC delta
    double ndc_dx = (2.0 * dx) / static_cast<double>(win->m_windowWidth);
    double ndc_dy = (-2.0 * dy) / static_cast<double>(win->m_windowHeight);

    // account for camera scale
    float off_dx = static_cast<float>(ndc_dx) / win->m_camScale;
    float off_dy = static_cast<float>(ndc_dy) / win->m_camScale;

    win->m_camOX += off_dx;
    win->m_camOY += off_dy;

    win->m_renderer.setCamera(win->m_camOX, win->m_camOY, win->m_camScale);
}

void Windower::m_scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Windower* win = reinterpret_cast<Windower*>(glfwGetWindowUserPointer(window));
    if (!win) return;

    double cx, cy;
    glfwGetCursorPos(window, &cx, &cy);

    // convert to NDC coordinates used by the shader
    double ndc_x = (2.0 * cx) / static_cast<double>(win->m_windowWidth) - 1.0;
    double ndc_y = -((2.0 * cy) / static_cast<double>(win->m_windowHeight) - 1.0);

    // compute scale factor
    double factor = std::pow(1.125, yoffset); // ~12.5% per wheel tick
    double oldScale = win->m_camScale;
    double newScale = oldScale * factor;

    
    double screennx = ndc_x;
    double screenny = ndc_y;

    win->m_camOX = static_cast<float>(win->m_camOX + screennx * (1.0/newScale - 1.0/oldScale));
    win->m_camOY = static_cast<float>(win->m_camOY + screenny * (1.0/newScale - 1.0/oldScale));
    win->m_camScale = static_cast<float>(newScale);

    win->m_renderer.setCamera(win->m_camOX, win->m_camOY, win->m_camScale);
}

void Windower::resizeViewport(GLFWwindow* window, int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
    glViewport(0, 0, m_windowWidth, m_windowHeight);
}

void Windower::m_framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    Windower* win = reinterpret_cast<Windower*>(glfwGetWindowUserPointer(window));
    if (win) win->resizeViewport(window, width, height);
}

Windower::~Windower() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}
