#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "map_data.hpp"
#include "a_star.hpp"

#include "windower.hpp"
#include "renderer.hpp"


int main(void)
{  
    // parseMap();
    // aStar();
    
    Renderer renderer;
    renderer.setVertices({
        0.5f, 0.5f, 0.0f, // top right
        0.5f, -0.5f, 0.0f, // bottom right
        -0.5f, 0.5f, 0.0f, // top left
        -0.5f, -0.5f, 0.0f, // bottom left
    });
    renderer.setIndices({
        0, 1, 2,
        1, 2, 3
    });
   

    Windower windower(renderer, 800, 640);
    windower.run();

}