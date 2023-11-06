#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

extern GLFWwindow* window;

struct Vertex2D
{
  float x, y; // Position
  float r, g, b; // Color
};

struct Matrix
{
  float a0, a1, a2, a3;
  float b0, b1, b2, b3;
  float c0, c1, c2, c3;
  float d0, d1, d2, d3;
};

// Triangle
extern const std::vector<Vertex2D> triangleVertices;
extern const std::vector<unsigned int> triangleIndices;
// Four triangles quad
extern const std::vector<Vertex2D> quadVertices;
extern const std::vector<unsigned int> quadIndices;

// Processes
bool startGL(int width, int height, char* title);
void stopGL();
// Shaders
std::string readShaderFile(const std::filesystem::path& shaderPath);
GLuint compileShader(const std::string& shaderCode, unsigned int type);
GLuint createRenderProgram(const std::filesystem::path& vertexShaderPath, const std::filesystem::path& fragmentShaderPath);
GLuint createComputeProgram(const std::filesystem::path& computeShaderPath);
bool checkShaderErrors(GLuint shader);
bool checkProgramErrors(GLuint program);
// Geometry
GLuint createBackgroundQuadVAO();
// Matrices
void setPositionMatrix(Matrix* matrix, const float x, const float y);