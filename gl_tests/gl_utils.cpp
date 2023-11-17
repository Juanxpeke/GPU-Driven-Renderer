#include <iostream>
#include <sstream>
#include <fstream>
#include <format>
#include "gl_utils.h"
#include "path_manager.h"

GLFWwindow* window;
std::string windowTitle;

const std::vector<Vertex2D_UV> triangleVerticesUV =
{
  // XY          // UV
  { 0.00f, 0.0f, 0.0f, 0.0f},
  { 0.05f, 0.1f, 0.5f, 1.0f},
  { 0.10f, 0.0f, 1.0f, 0.0f}
};

const std::vector<Vertex2D_RGB> triangleVerticesRGB =
{
  // XY          // RGB
  { 0.00f, 0.0f, 1.0f, 0.0f, 0.0f, },
  { 0.05f, 0.1f, 0.0f, 1.0f, 0.0f, },
  { 0.10f, 0.0f, 0.0f, 0.0f, 1.0f, }
};

const std::vector<unsigned int> triangleIndices =
{
  0, 1, 2
};

const std::vector<Vertex2D_UV> quadVerticesUV =
{
  // XY           // UV
  { 0.00f, 0.00f,	0.0f, 0.0f },
  { 0.10f, 0.00f,	1.0f, 0.0f },
  { 0.05f, 0.05f, 0.5f, 0.5f },
  { 0.00f, 0.10f,	0.0f, 1.0f },
  { 0.10f, 0.10f,	1.0f, 1.0f }
};

const std::vector<Vertex2D_RGB> quadVerticesRGB =
{
  // XY			      // RGB
  { 0.00f, 0.00f,	1.0f, 0.0f, 0.0f, }, // Bottom left
  { 0.10f, 0.00f,	0.0f, 1.0f, 0.0f, }, // Bottom right
  { 0.05f, 0.05f, 0.0f, 0.0f, 1.0f, }, // Center
  { 0.00f, 0.10f,	1.0f, 1.0f, 0.0f, }, // Top left
  { 0.10f, 0.10f,	1.0f, 1.0f, 1.0f, }  // Top right
};

const std::vector<unsigned int> quadIndices =
{
  0, 1, 2, // Bottom triangle
  1, 4, 2, // Right triangle
  2, 4, 3, // Top triangle
  0, 2, 3  // Left triangle
};

extern const std::vector<float> billboardQuadVertices =
{ 
  // XY
  -0.5f, -0.5f,
   0.5f, -0.5f,
  -0.5f,  0.5f,
   0.5f,  0.5f,
};

extern const std::vector<unsigned int> billboardQuadIndices =
{ 
  0, 1, 2, // Bottom left
  1, 2, 3  // Top right
};

// ====
// GLFW
// ====

bool startGL(int width, int height, const char* title) 
{
  { // GLFW
    if (!glfwInit())
    {
      fprintf(stderr, "ERROR: could not start GLFW3\n");
      return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title, NULL, NULL);
    windowTitle = title;

    if (!window)
    {
      fprintf(stderr, "ERROR: could not open window with GLFW3\n" );
      glfwTerminate();
      return false;
    }

    glfwMakeContextCurrent(window);

#if DISABLE_FPS_CAP
    glfwSwapInterval(0);
#endif
  }

  { // glad
    gladLoadGL();
  }

  const GLubyte* renderer = glGetString(GL_RENDERER);
  const GLubyte* version = glGetString(GL_VERSION);

  printf("Renderer: %s\n", renderer);
  printf("OpenGL version %s\n", version);

  return true;
}

void stopGL() { glfwTerminate(); }

// =======
// Shaders
// =======

std::string readShaderFile(const std::filesystem::path& shaderPath)
{
  std::ifstream shaderFileStream(shaderPath);

  if (!shaderFileStream.good())
  {
    return std::string();
  }

  std::stringstream shaderStringStream;
  shaderStringStream << shaderFileStream.rdbuf();
  shaderFileStream.close();

  return shaderStringStream.str();
}

GLuint compileShader(const std::string& shaderCode, unsigned int type)
{
  const char* shaderSource = shaderCode.c_str();
  
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &shaderSource, 0);
  glCompileShader(shader);
  checkShaderErrors(shader);

  return shader;
}

GLuint createRenderProgram(const std::filesystem::path& vertexShaderPath, const std::filesystem::path& fragmentShaderPath)
{
  GLuint program = glCreateProgram();

  std::string vertexShaderCode = readShaderFile(vertexShaderPath);
  GLuint vertexShader = compileShader(vertexShaderCode, GL_VERTEX_SHADER); 
  glAttachShader(program, vertexShader);

  std::string fragmentShaderCode = readShaderFile(fragmentShaderPath);
  GLuint fragmentShader = compileShader(fragmentShaderCode, GL_FRAGMENT_SHADER); 
  glAttachShader(program, fragmentShader);

  glLinkProgram(program);
  checkProgramErrors(program);

  return program;
}

GLuint createComputeProgram(const std::filesystem::path& computeShaderPath)
{
  GLuint program = glCreateProgram();

  std::string computeShaderCode = readShaderFile(computeShaderPath);
  GLuint computeShader = compileShader(computeShaderCode, GL_COMPUTE_SHADER); 
  glAttachShader(program, computeShader);

  glLinkProgram(program);
  checkProgramErrors(program);

  return program;
}

bool checkShaderErrors(GLuint shader)
{
  GLint params = -1;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &params);
  if (GL_TRUE != params)
  {
    fprintf(stderr, "ERROR: shader %u did not compile\n", shader);
    return false;
  }
  return true;
}

bool checkProgramErrors(GLuint program)
{
  GLint params = -1;
  glGetProgramiv(program, GL_LINK_STATUS, &params);
  if (GL_TRUE != params)
  {
    fprintf(stderr, "ERROR: program %u did not link\n", program);
    return false;
  }
  return true;
}

// ========
// Geometry
// ========

GLuint createBackgroundQuadVAO()
{
  GLuint VAO = 0, VBO = 0;

  float vertices[] =
  { // XY         // UV
    -1.0f, -1.0f, 0.0f, 0.0f,
    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f
  };
  
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), vertices, GL_STATIC_DRAW);

  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  glEnableVertexAttribArray(0);

  GLintptr stride = 4 * sizeof(float);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, NULL);
  glEnableVertexAttribArray(1);
  
  GLintptr offset = 2 * sizeof(float);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*) offset);

  return VAO;
}

// ========
// Matrices
// ========

void setPositionMatrix(Matrix* matrix, const float x, const float y)
{
  /*
  1 0 0 0
  0 1 0 0
  0 0 1 0
  x y 0 1
  */
  matrix->a0 = 1;
  matrix->a1 = matrix->a2 = matrix->a3 = 0;

  matrix->b1 = 1;
  matrix->b0 = matrix->b2 = matrix->b3 = 0;

  matrix->c2 = 1;
  matrix->c0 = matrix->c1 = matrix->c3 = 0;

  matrix->d0 = x;
  matrix->d1 = y;
  matrix->d2 = 0;
  matrix->d3 = 1;
}

// =========
// Profiling
// =========

namespace
{
  double lastTime = 0.0;
  double dt = 0.0;

  double firstPreviousFPS = 0.0;
  double secondPreviousFPS = 0.0;
  double thirdPreviousFPS = 0.0;
  double fourthPreviousFPS = 0.0;
  double meanFPS = 0.0;

  double lastTitleChangeTime = 0.0;
  double titleChangeDelta = 0.5;
}

void updateProfiler()
{
  double currentTime = glfwGetTime();
  dt = currentTime - lastTime;
  lastTime = currentTime;

  if (dt == 0.0) return;

  fourthPreviousFPS = thirdPreviousFPS;
  thirdPreviousFPS = secondPreviousFPS;
  secondPreviousFPS = firstPreviousFPS;
  firstPreviousFPS = 1 / dt;

  meanFPS = (firstPreviousFPS + secondPreviousFPS + thirdPreviousFPS + fourthPreviousFPS) / 4.0;

  if (lastTime - lastTitleChangeTime >= titleChangeDelta)
  {
    lastTitleChangeTime = lastTime;

    std::string fpsString = std::format("{:.0f}", meanFPS);
    std::string titleString = windowTitle + " (" + fpsString + " FPS)";
    glfwSetWindowTitle(window, titleString.c_str());
  }
}

double getDeltaTime()
{
  return dt;
}

double getFPS()
{
  return meanFPS;
}