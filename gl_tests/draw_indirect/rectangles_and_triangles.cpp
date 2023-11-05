// Based on: Jakob Törmä Ruhl
#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "gl_utils.h"
#include "path_manager.h"

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);

// Settings
char title[256];
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;


namespace // Unnamed namespace
{
  struct Vertex2D
  {
    float x, y;  // Position
    float u, v;  // UV
  };

  struct Matrix
  {
    float a0, a1, a2, a3;
    float b0, b1, b2, b3;
    float c0, c1, c2, c3;
    float d0, d1, d2, d3;
  };

  void setMatrix(Matrix* matrix, const float x, const float y)
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

  struct DrawElementsCommand
  {
    GLuint vertexCount;
    GLuint instanceCount;
    GLuint firstIndex;
    GLuint baseVertex;
    GLuint baseInstance;
  };

  const std::vector<Vertex2D> gQuad = {
    //xy			      //uv
    { 0.00f, 0.00f,	0.0f, 0.0f },
    { 0.10f, 0.00f,	1.0f, 0.0f },
    { 0.05f, 0.05f, 0.5f, 0.5f },
    { 0.00f, 0.10f,	0.0f, 1.0f },
    { 0.10f, 0.10f,	1.0f, 1.0f }
  };

  const std::vector<Vertex2D> gTriangle =
  {
    { 0.00f, 0.0f, 0.0f, 0.0f},
    { 0.05f, 0.1f, 0.5f, 1.0f},
    { 0.10f, 0.0f, 1.0f, 0.0f}
  };

  const std::vector<unsigned int> gQuadIndex = {
    0, 1, 2,
    1, 4, 2,
    2, 4, 3,
    0, 2, 3
  };

  const std::vector<unsigned int> gTriangleIndex =
  {
    0, 1, 2
  };

  GLuint gVAO(0);
  GLuint gArrayTexture(0);
  GLuint gVertexBuffer(0);
  GLuint gElementBuffer(0);
  GLuint gIndirectBuffer(0);
  GLuint gMatrixBuffer(0);
  GLuint gProgram(0);

  float gMouseX(0);
  float gMouseY(0);

} // Unnamed namespace

void generateGeometry()
{
  // Generate 50 quads, 50 triangles
  const unsigned numVertices = gQuad.size() * 50 + gTriangle.size() * 50;
  std::vector<Vertex2D> vVertex(numVertices);

  Matrix vMatrix[100];

  unsigned vertexIndex(0);
  unsigned matrixIndex(0);

  // Clipspace, lower left corner = (-1, -1)
  float xOffset(-0.95f);
  float yOffset(-0.95f);

  // Populate geometry
  for (unsigned int i(0); i != 10; ++i)
  {
    for (unsigned int j(0); j != 10; ++j)
    {
      // Quad
      if (j % 2 == 0)
      {
        for (unsigned int k(0); k != gQuad.size(); ++k)
        {
          vVertex[vertexIndex++] = gQuad[k];
        }
      }
      // Triangle
      else
      {
        for (unsigned int k(0); k != gTriangle.size(); ++k)
        {
          vVertex[vertexIndex++] = gTriangle[k];
        }
      }

      // Set position in model matrix
      setMatrix(&vMatrix[matrixIndex++], xOffset, yOffset);
      xOffset += 0.2f;
    }
    yOffset += 0.2f;
    xOffset = -0.95f;
  }

  glGenVertexArrays(1, &gVAO);
  glBindVertexArray(gVAO);

  glGenBuffers(1, &gVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * vVertex.size(), vVertex.data(), GL_STATIC_DRAW);

  // Specify vertex attributes for the shader
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (GLvoid*) (offsetof(Vertex2D, x)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (GLvoid*) (offsetof(Vertex2D, u)));

  // Create an element buffer and populate it
  int triangleBytes = sizeof(unsigned int) * gTriangleIndex.size();
  int quadBytes = sizeof(unsigned int) * gQuadIndex.size();

  glGenBuffers(1, &gElementBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gElementBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangleBytes + quadBytes, NULL, GL_STATIC_DRAW);

  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, quadBytes, gQuadIndex.data());
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, quadBytes, triangleBytes, gTriangleIndex.data());

  // Setup per instance matrices
  // Method 1. Use Vertex attributes and the vertex attrib divisor
  glGenBuffers(1, &gMatrixBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, gMatrixBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vMatrix), vMatrix, GL_STATIC_DRAW);
  // A matrix is 4 vec4s
  glEnableVertexAttribArray(3 + 0);
  glEnableVertexAttribArray(3 + 1);
  glEnableVertexAttribArray(3 + 2);
  glEnableVertexAttribArray(3 + 3);

  glVertexAttribPointer(3 + 0, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix), (GLvoid*) (offsetof(Matrix, a0)));
  glVertexAttribPointer(3 + 1, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix), (GLvoid*) (offsetof(Matrix, b0)));
  glVertexAttribPointer(3 + 2, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix), (GLvoid*) (offsetof(Matrix, c0)));
  glVertexAttribPointer(3 + 3, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix), (GLvoid*) (offsetof(Matrix, d0)));
  // Only apply one per instance
  glVertexAttribDivisor(3 + 0, 1);
  glVertexAttribDivisor(3 + 1, 1);
  glVertexAttribDivisor(3 + 2, 1);
  glVertexAttribDivisor(3 + 3, 1);

  // Method 2. Use Uniform Buffers. Not shown here
}

void generateArrayTexture()
{
  // Generate an array texture
  glGenTextures(1, &gArrayTexture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, gArrayTexture);

  // Create storage for the texture. (100 layers of 1x1 texels)
  glTexStorage3D(
      GL_TEXTURE_2D_ARRAY,
      1, // No mipmaps as textures are 1x1
      GL_RGB8, // Internal format
      1, 1, // Width, height
      100); //Number of layers

  for (unsigned int i(0); i != 100; ++i)
  {
    // Choose a random color for the i-essim image
    GLubyte color[3] = { GLubyte(rand() % 255),GLubyte(rand() % 255),GLubyte(rand() % 255) };

    // Specify i-essim image
    glTexSubImage3D(
        GL_TEXTURE_2D_ARRAY,
        0, // Mipmap number
        0, 0, i, // xoffset, yoffset, zoffset
        1, 1, 1, // Width, height, depth
        GL_RGB, // Format
        GL_UNSIGNED_BYTE, // Type
        color); // Pointer to data
  }

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void generateDrawCommands()
{
  // Generate draw commands
  DrawElementsCommand vDrawCommand[100];
  GLuint baseVert = 0;
  for (unsigned int i(0); i<100; ++i)
  {
    // Quad
    if (i % 2 == 0)
    {
      vDrawCommand[i].vertexCount = 12; // 4 triangles = 12 vertices
      vDrawCommand[i].instanceCount = 1; // Draw 1 instance
      vDrawCommand[i].firstIndex = 0; // Draw from index 0 for this instance
      vDrawCommand[i].baseVertex = baseVert; // Starting from baseVert
      vDrawCommand[i].baseInstance = i; // gl_InstanceID
      baseVert += gQuad.size();
    }
    // Triangle
    else
    {
      vDrawCommand[i].vertexCount = 3; // 1 triangle = 3 vertices
      vDrawCommand[i].instanceCount = 1; // Draw 1 instance
      vDrawCommand[i].firstIndex = 0; // Draw from index 0 for this instance
      vDrawCommand[i].baseVertex = baseVert; // Starting from baseVert
      vDrawCommand[i].baseInstance = i; // gl_InstanceID
      baseVert += gTriangle.size();
    }
  }

  // Feed the draw command data to the GPU
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gIndirectBuffer);
  glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(vDrawCommand), vDrawCommand, GL_DYNAMIC_DRAW);

  // Feed the instance id to the shader.
  glBindBuffer(GL_ARRAY_BUFFER, gIndirectBuffer);
  glEnableVertexAttribArray(2);
  glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(DrawElementsCommand), (void*) (offsetof(DrawElementsCommand, baseInstance)));
  glVertexAttribDivisor(2, 1); // Only once per instance
}

int main()
{
  sprintf(title, "Rectangles and triangles (MultiDrawElementsIndirect)");
  startGL(WIDTH, HEIGHT, title);

  // Set clear color
  glClearColor(1.0, 1.0, 1.0, 0.0);

  // Create and bind the shader program
  gProgram = createRenderProgram(shaderPath("rect.vert"), shaderPath("rect.frag"));
  glUseProgram(gProgram);

  generateGeometry();
  generateArrayTexture();

  // Set the sampler for the texture.
  // Hacky but we know that the arraysampler is at bindingpoint 0.
  glUniform1i(0, 0);

  // Generate one indirect draw buffer
  glGenBuffers(1, &gIndirectBuffer);

  // Render loop
  while (!glfwWindowShouldClose(window))
  {
    glClear(GL_COLOR_BUFFER_BIT);

    // Use program. Not needed in this example since we only have one that
    // we already use
    // glUseProgram(gProgram);

    // Bind the vertex array we want to draw from. Not needed in this example
    // since we only have one that is already bounded
    // glBindVertexArray(gVAO);

    generateDrawCommands();

    // Populate light uniform
    glUniform2f(glGetUniformLocation(gProgram, "light_pos"), gMouseX, gMouseY);

    // Draw
    glMultiDrawElementsIndirect(
        GL_TRIANGLES, // Type
        GL_UNSIGNED_INT, // Indices represented as unsigned ints
        (GLvoid*) 0, // Start with the first draw command
        100, // Draw 100 objects
        0); // No stride, the draw commands are tightly packed


    if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_ESCAPE))
    {
      glfwSetWindowShouldClose(window, 1);
    }

    // GLFW: Swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // GLFW: Terminate, clearing all previously allocated GLFW resources.
  glfwTerminate();

  // Clean-up
  glDeleteProgram(gProgram);
  glDeleteVertexArrays(1, &gVAO);
  glDeleteBuffers(1, &gVertexBuffer);
  glDeleteBuffers(1, &gElementBuffer);
  glDeleteBuffers(1, &gMatrixBuffer);
  glDeleteBuffers(1, &gIndirectBuffer);
  glDeleteTextures(1, &gArrayTexture);
  return 0;
}

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
  gMouseX = -0.5f + float(xpos) / float(WIDTH);
  gMouseY = 0.5f - float(ypos) / float(HEIGHT);
}