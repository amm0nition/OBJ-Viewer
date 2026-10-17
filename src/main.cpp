#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

#include "shaders.h"

struct Vertex
{
    float x, y, z;
    float nx, ny, nz;
};

struct Mesh
{
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    GLsizei vertexCount = 0;
    std::string name;
};

Mesh g_mesh;
float g_rotX = 20.0f;
float g_rotY = -30.0f;
float g_zoom = 3.0f;
bool g_autoRotate = true;
bool g_mouseDragging = false;
double g_lastMouseX = 0.0;
double g_lastMouseY = 0.0;

bool loadOBJ(const std::string& path, Mesh& mesh)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec3> temp_normals;

    struct FaceVertex
    {
        int v = 0;
        int vt = 0;
        int vn = 0;
    };

    std::vector<Vertex> vertices;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v")
        {
            glm::vec3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            temp_positions.push_back(pos);
        }
        else if (prefix == "vn")
        {
            glm::vec3 norm;
            ss >> norm.x >> norm.y >> norm.z;
            temp_normals.push_back(glm::normalize(norm));
        }
        else if (prefix == "f")
        {
            std::vector<FaceVertex> face;
            std::string token;
            while (ss >> token)
            {
                FaceVertex fv;
                std::replace(token.begin(), token.end(), '/', ' ');
                std::istringstream tokenStream(token);
                tokenStream >> fv.v;
                if (token.find("  ") != std::string::npos)
                {
                    tokenStream >> fv.vn;
                }
                else
                {
                    if (tokenStream >> fv.vt)
                        tokenStream >> fv.vn;
                }
                face.push_back(fv);
            }

            if (face.size() < 3)
                continue;

            auto resolveIndex = [](int idx, size_t count) -> int {
                if (idx > 0) return idx - 1;
                if (idx < 0) return (int)count + idx;
                return -1;
            };

            for (size_t i = 1; i + 1 < face.size(); ++i)
            {
                FaceVertex fv[3] = { face[0], face[i], face[i + 1] };
                glm::vec3 p[3];
                glm::vec3 n[3];
                bool hasNormal[3] = { false, false, false };

                for (int k = 0; k < 3; ++k)
                {
                    int vIdx = resolveIndex(fv[k].v, temp_positions.size());
                    p[k] = (vIdx >= 0 && vIdx < (int)temp_positions.size()) ? temp_positions[vIdx] : glm::vec3(0.0f);

                    int nIdx = resolveIndex(fv[k].vn, temp_normals.size());
                    if (nIdx >= 0 && nIdx < (int)temp_normals.size())
                    {
                        n[k] = temp_normals[nIdx];
                        hasNormal[k] = true;
                    }
                }

                glm::vec3 faceNormal = glm::cross(p[1] - p[0], p[2] - p[0]);
                float len = glm::length(faceNormal);
                faceNormal = (len > 0.00001f) ? (faceNormal / len) : glm::vec3(0.0f, 1.0f, 0.0f);

                for (int k = 0; k < 3; ++k)
                {
                    glm::vec3 finalNorm = hasNormal[k] ? n[k] : faceNormal;
                    vertices.push_back({ p[k].x, p[k].y, p[k].z, finalNorm.x, finalNorm.y, finalNorm.z });
                }
            }
        }
    }

    if (vertices.empty())
        return false;

    glm::vec3 minBound = glm::vec3(vertices[0].x, vertices[0].y, vertices[0].z);
    glm::vec3 maxBound = minBound;

    for (const auto& v : vertices)
    {
        minBound.x = std::min(minBound.x, v.x);
        minBound.y = std::min(minBound.y, v.y);
        minBound.z = std::min(minBound.z, v.z);
        maxBound.x = std::max(maxBound.x, v.x);
        maxBound.y = std::max(maxBound.y, v.y);
        maxBound.z = std::max(maxBound.z, v.z);
    }

    glm::vec3 center = (minBound + maxBound) * 0.5f;
    glm::vec3 size = maxBound - minBound;
    float maxDim = std::max(size.x, std::max(size.y, size.z));
    float scale = (maxDim > 0.0001f) ? (2.0f / maxDim) : 1.0f;

    for (auto& v : vertices)
    {
        v.x = (v.x - center.x) * scale;
        v.y = (v.y - center.y) * scale;
        v.z = (v.z - center.z) * scale;
    }

    if (mesh.VAO == 0)
        glGenVertexArrays(1, &mesh.VAO);
    if (mesh.VBO == 0)
        glGenBuffers(1, &mesh.VBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    mesh.vertexCount = (GLsizei)vertices.size();
    mesh.name = path;
    return true;
}

std::string getExeDirectory()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string path(buffer);
    size_t pos = path.find_last_of("\\/");
    return (pos != std::string::npos) ? path.substr(0, pos) : ".";
}

std::string findObjFile(const std::string& dir)
{
    std::string defaultPath = dir + "\\utah_teapot.obj";
    DWORD attribs = GetFileAttributesA(defaultPath.c_str());
    if (attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY))
        return defaultPath;

    WIN32_FIND_DATAA findData;
    std::string searchPattern = dir + "\\*.obj";
    HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        std::string found = dir + "\\" + findData.cFileName;
        FindClose(hFind);
        return found;
    }

    return "";
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    if (count > 0)
    {
        if (loadOBJ(paths[0], g_mesh))
        {
            std::string title = "OBJ Viewer - " + g_mesh.name;
            glfwSetWindowTitle(window, title.c_str());
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            g_mouseDragging = true;
            glfwGetCursorPos(window, &g_lastMouseX, &g_lastMouseY);
        }
        else if (action == GLFW_RELEASE)
        {
            g_mouseDragging = false;
        }
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (g_mouseDragging)
    {
        float dx = (float)(xpos - g_lastMouseX);
        float dy = (float)(ypos - g_lastMouseY);
        g_rotY += dx * 0.5f;
        g_rotX += dy * 0.5f;
        g_rotX = std::max(-89.0f, std::min(89.0f, g_rotX));
        g_lastMouseX = xpos;
        g_lastMouseY = ypos;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_zoom -= (float)yoffset * 0.25f;
    g_zoom = std::max(0.5f, std::min(15.0f, g_zoom));
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(window, true);
        else if (key == GLFW_KEY_SPACE)
            g_autoRotate = !g_autoRotate;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main(int argc, char** argv)
{
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "OBJ Viewer", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetDropCallback(window, drop_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::string modelPath;
    if (argc > 1)
        modelPath = argv[1];
    else
        modelPath = findObjFile(getExeDirectory());

    if (!modelPath.empty() && loadOBJ(modelPath, g_mesh))
    {
        std::string title = "OBJ Viewer - " + modelPath;
        glfwSetWindowTitle(window, title.c_str());
    }
    else
    {
        glfwSetWindowTitle(window, "OBJ Viewer - Drag and Drop .obj file here");
    }

    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int lightDirLoc = glGetUniformLocation(shaderProgram, "lightDir");
    int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    int objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    int lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (g_autoRotate && !g_mouseDragging)
            g_rotY += (float)deltaTime * 30.0f;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (g_mesh.vertexCount > 0)
        {
            glUseProgram(shaderProgram);

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::rotate(model, glm::radians(g_rotX), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(g_rotY), glm::vec3(0.0f, 1.0f, 0.0f));

            glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -g_zoom));
            glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 100.0f);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

            glUniform3f(lightDirLoc, 1.0f, 1.5f, 2.0f);
            glUniform3f(viewPosLoc, 0.0f, 0.0f, g_zoom);
            glUniform3f(objectColorLoc, 0.85f, 0.85f, 0.88f);
            glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);

            glBindVertexArray(g_mesh.VAO);
            glDrawArrays(GL_TRIANGLES, 0, g_mesh.vertexCount);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (g_mesh.VAO)
        glDeleteVertexArrays(1, &g_mesh.VAO);
    if (g_mesh.VBO)
        glDeleteBuffers(1, &g_mesh.VBO);

    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
