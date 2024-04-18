#include "graphics_batch.h"

#include <glad/glad.h>

struct DrawElementsIndirectCommand
{
  uint32_t count = 0;         // # Elements (i.e indices)
  uint32_t instanceCount = 0; // # Instances (kind of draw calls)
  uint32_t firstIndex = 0;    // Index of the first element in the EBO
  uint32_t baseVertex = 0;    // Base vertex when reading from vertex buffer
  uint32_t baseInstance = 0;  // Base instance when using gl_InstanceID
  uint32_t padding0 = 0;      // Padding due to GLSL layout std140 16B alignment rule
  uint32_t padding1 = 0;
  uint32_t padding2 = 0;
};

GraphicsBatch::GraphicsBatch(std::shared_ptr<Mesh> mesh, uint32_t shader) : meshPtr(mesh), shaderID(shader)
{
  unsigned int IBO, modelSSBO, materialSSBO;
  
  glGenBuffers(1, &IBO);
  glGenBuffers(1, &modelSSBO);
  // glGenBuffers(1, &materialSSBO);

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, IBO);
  glBufferData(GL_DRAW_INDIRECT_BUFFER, 1 * sizeof(DrawElementsIndirectCommand), nullptr, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, INITIAL_INSTANCES_COUNT * 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

  // glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
  // glBufferData(GL_SHADER_STORAGE_BUFFER, INITIAL_INSTANCES_COUNT * sizeof(Material), nullptr, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  materialSSBO = 0;

  indirectBufferID = IBO;
  // std::cout << "MODEL SSBO: " << modelSSBO << std::endl;
  modelBufferID = modelSSBO;
  materialBufferID = materialSSBO;
}

uint32_t GraphicsBatch::getMeshIndexCount() const noexcept
{
  return meshPtr->getIndexBufferCount();
}

uint32_t GraphicsBatch::getMeshVAO() const noexcept
{
  return meshPtr->getVertexArrayID();
}

uint32_t GraphicsBatch::getShaderID() const noexcept
{
  return shaderID;
}

void GraphicsBatch::updateBuffers() const
{
  updateIndirectBuffer();
  updateModelBuffer(); // TODO, URGENT: THIS LEADS TO A BOTTLENECK
}

void GraphicsBatch::updateIndirectBuffer() const
{
  DrawElementsIndirectCommand drawCommands[1];

  drawCommands[0].count = getMeshIndexCount();
  drawCommands[0].instanceCount = meshInstances.size();
  drawCommands[0].firstIndex = 0;
  drawCommands[0].baseVertex = 0;
  drawCommands[0].baseInstance = 0;

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBufferID);
  glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, 1 * sizeof(DrawElementsIndirectCommand), drawCommands);
}

void GraphicsBatch::updateModelBuffer() const
{
  std::vector<float> modelMatrices;

  for (int i = 0; i < meshInstances.size(); i++)
  {
    const MeshInstance& meshInstance = meshInstances[i];
    glm::mat4 model = meshInstance.getModelMatrix();
    for (int x = 0; x < 4; x++){
      for (int y = 0; y < 4; y++) {
        modelMatrices.push_back(model[x][y]);
      }
    }
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelBufferID);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, modelMatrices.size() * sizeof(float), modelMatrices.data());
}
