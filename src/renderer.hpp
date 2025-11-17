#ifndef RENDERER
#define RENDERER

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>



class Renderer {
private:

    std::vector<float> m_vertices;
    std::vector<unsigned int> m_indices;

    std::string m_vertexShaderSource;
    std::string m_fragmentShaderSource;

    GLuint m_shaderProgram;

    void readShader(const std::string& filepath);
    GLuint createShader(GLenum type, const std::string& source);
    GLuint linkShadersIntoProgram(const std::vector<GLuint>&& shaders);
    

public:

    Renderer();
    ~Renderer() = default;

    void setVertices(const std::vector<float>& arr) { m_vertices = arr; }
    void setIndices(const std::vector<unsigned int>& arr) { m_indices = arr; }

    void render() const;
    void defineGeometry();
};

#endif