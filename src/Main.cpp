#include <iostream>
#include "../deps/imgui/imgui.h"
#include "../deps/imgui/imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>

#include "Client/Graphics/Shader.h"
#include "Client/Graphics/Camera.h"
#include "Client/Data/Chunk.h"
#include <glm/gtx/string_cast.hpp>
//#include <FastNoise/FastNoise.h>



void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool notRandomBool() {
    return 0 + (rand() % (1 - 0 + 1)) == 1;
}

float RandomFloat(float a, float b) {
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "minecraft clone", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader ourShader("res/Shader/Cube.vert", "res/Shader/Cube.frag");

    float vertices[] = {
            -0.5f, -0.5f, -0.5f,                      //right -z
            0.5f, -0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f,                     // +z left
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            -0.5f,  0.5f,  0.5f,                      // -x back
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,

            0.5f,  0.5f,  0.5f,                               // +x front
            0.5f,  0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,

            -0.5f, -0.5f, -0.5f,                      // -y down
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f,  0.5f, -0.5f,                   // +y up
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f
    };

    float test[] = {
            -0.5f, -0.5f, -0.5f,  //right -z
            0.5f, -0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f,  0.5f, // +z left
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            -0.5f,  0.5f,  0.5f,  // -x back
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,

            0.5f,  0.5f,  0.5f,  // +x front
            0.5f,  0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,

            -0.5f, -0.5f, -0.5f, // -y down
            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f,  0.5f,
            0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f,  0.5f, -0.5f, // +y up
            0.5f,  0.5f, -0.5f,
            0.5f,  0.5f,  0.5f,
            0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f
    };

    for(int i = 0; i < 36; i++) {
        test[0 + 3*i] += 0.0f;
        test[1 + 3*i] += 1.0f;
        test[2 + 3*i] += 0.0f;
    }

    /*for(int i = 0; i < 108; i++) {
        std::cout << test[i] << std::endl;
    }*/


    bool chunk[16][16][16];
    for(int x = 0; x < 16; x++) {
        for(int y = 0; y < 16; y++) {
            for(int z = 0; z < 16; z++) {
                chunk[x][y][z] = notRandomBool();
            }
        }
    }

    glm::vec3 color[16][16][16];
    for(int x = 0; x < 16; x++) {
        for(int y = 0; y < 16; y++) {
            for(int z = 0; z < 16; z++) {
                float r = RandomFloat(0.0f, 1.0f);
                float g = RandomFloat(0.0f, 1.0f);
                float b = RandomFloat(0.0f, 1.0f);
                color[x][y][z] = {r, g, b};
            }
        }
    }

    Chunk bruh;

    unsigned int bruhVBO, bruhVAO;
    glGenVertexArrays(1, &bruhVAO);
    glGenBuffers(1, &bruhVBO);

    glBindVertexArray(bruhVAO);

    auto mesh = bruh.GetMesh();

    for(int i = 0; i < mesh.size(); i++) {
        std::cout << mesh[i] << std::endl;
    }
    glBindBuffer(GL_ARRAY_BUFFER, bruhVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mesh), &mesh[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    glBindVertexArray(0);

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    // texture coord attribute
    /*glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);*/

    glBindVertexArray(0);

    Camera camera(SCR_WIDTH, SCR_HEIGHT, {3.0f, 3.0f, 3.0f});


    ourShader.Bind();
    ourShader.SetMat4("projection", camera.GetProjection());

    unsigned int testVBO, testVAO;
    glGenVertexArrays(1, &testVAO);
    glGenBuffers(1, &testVBO);

    glBindVertexArray(testVAO);

    glBindBuffer(GL_ARRAY_BUFFER, testVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(test), test, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);


    bool mode = false;

    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        if (glfwGetKey(window, GLFW_KEY_C) != GLFW_RELEASE) {
            if(mode) {
                glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
                mode = false;
            }

            else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                mode = true;
            }
        }

        //std::cout << glm::to_string(camera.GetPosition()) << std::endl;

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.Inputs(window);

        ourShader.Bind();
        ourShader.SetMat4("view", camera.GetView());

        glBindVertexArray(VAO);


        for(int x = 0; x < 16; x++) {
            for(int y = 0; y < 16; y++) {
                for(int z = 0; z < 16; z++) {
                    if(chunk[x][y][z]) {
                        glm::mat4 model = glm::mat4(1.0f);
                        model = glm::translate(model, {1.0f + x, 1.0f + y, 1.0f +z});
                        ourShader.SetMat4("model", model);
                        //std::cout << glm::to_string(color[x][y][z]) << std::endl;

                        ourShader.SetFloat3("color", color[x][y][z]);

                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                }
            }
        }

        /*glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, {1.0f, 1.0f, 1.0f});
        ourShader.SetMat4("model", model);
        ourShader.SetFloat3("color", {0.8f, 0.5f, 0.2f});

        glDrawArrays(GL_TRIANGLES, 0, 36);

        model = glm::mat4(1.0f);
        model = glm::translate(model, {4.0f, 1.0f, 1.0f});
        ourShader.SetMat4("model", model);
        ourShader.SetFloat3("color", {0.8f, 0.5f, 0.2f});

        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(testVAO);
        model = glm::mat4(1.0f);
        model = glm::translate(model, {1.0f, 1.0f, 1.0f});
        ourShader.SetMat4("model", model);
        ourShader.SetFloat3("color", {0.8f, 0.5f, 0.2f});
        glDrawArrays(GL_TRIANGLES, 0, 36);*/

        /*glBindVertexArray(VAO);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, {1.0f, 1.0f, 1.0f});
        ourShader.SetMat4("model", model);
        ourShader.SetFloat3("color", {0.8f, 0.5f, 0.2f});

        glDrawArrays(GL_TRIANGLES, 0, 36);*/

        //for(int i = 0)

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}