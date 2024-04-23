#include "renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../path_manager.h"
#include "mesh.h"
#include "diffuse_flat_material.h"
#include "diffuse_textured_material.h"

void Renderer::startUp() noexcept
{
  shaders[static_cast<unsigned int>(MaterialType::DiffuseFlat)] = ShaderProgram(shaderPath("diffuse_flat.vert"), shaderPath("diffuse_flat.frag"));
  shaders[static_cast<unsigned int>(MaterialType::DiffuseTextured)] = ShaderProgram(shaderPath("diffuse_textured.vert"), shaderPath("diffuse_textured.frag"));

  glEnable(GL_DEPTH_TEST);

  glGenBuffers(1, &lightsDataUBO);
  glBindBuffer(GL_UNIFORM_BUFFER, lightsDataUBO);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(LightsData), NULL, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightsDataUBO);
}

void Renderer::shutDown() noexcept
{
  glDeleteBuffers(1, &lightsDataUBO);
}

void Renderer::render(Camera& camera) noexcept
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix;
  glm::vec3 cameraPosition = glm::vec3(0.0f);

  LightsData lightsData;

	lightsData.ambientLight = ambientLight;

  uint32_t directionalLightsCount = std::min(static_cast<uint32_t>(NUM_HALF_MAX_DIRECTIONAL_LIGHTS * 2), static_cast<uint32_t>(directionalLights.size()));
  lightsData.directionalLightsCount = static_cast<int>(directionalLightsCount);

  for (uint32_t i = 0; i < directionalLightsCount; i++)
  {
    const DirectionalLight& dirLight = directionalLights[i];
    lightsData.directionalLights[i].colorIntensity = dirLight.getLightColor() * dirLight.getLightIntensity();
    lightsData.directionalLights[i].direction = glm::rotate(dirLight.getLightDirection(), dirLight.getFrontVector());
  }

  uint32_t pointLightsCount = std::min(static_cast<uint32_t>(NUM_HALF_MAX_POINT_LIGHTS * 2), static_cast<uint32_t>(pointLights.size()));
  lightsData.pointLightsCount = static_cast<int>(pointLightsCount);

  for (uint32_t i = 0; i < pointLightsCount; i++)
  {
    const PointLight& pointLight = pointLights[i];
    lightsData.pointLights[i].colorIntensity = pointLight.getLightColor() * pointLight.getLightIntensity();
    lightsData.pointLights[i].position = pointLight.getLocalTranslation();
    lightsData.pointLights[i].radius = pointLight.getMaxRadius();
  }

	glBindBuffer(GL_UNIFORM_BUFFER, lightsDataUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightsData), &lightsData);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

  for (int i = 0; i < meshInstances.size(); i++)
  {
    const MeshInstance& meshInstance = meshInstances[i];
    meshInstance.getMaterial()->setUniforms(camera.getProjectionMatrix(), camera.getViewMatrix(), meshInstance.getModelMatrix(), camera.getLocalTranslation());
    glBindVertexArray(meshInstance.getMeshVAO());
		glDrawElements(GL_TRIANGLES, meshInstance.getMeshIndexCount(), GL_UNSIGNED_INT, nullptr);
  }
}

DirectionalLight* Renderer::createDirectionalLight()
{
  directionalLights.emplace_back();
  return &directionalLights.back();
}

PointLight* Renderer::createPointLight()
{
  pointLights.emplace_back();
  return &pointLights.back();
}

MeshInstance* Renderer::createMeshInstance(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
{
  meshInstances.emplace_back(mesh, material);
  return &meshInstances.back();
}

std::shared_ptr<Material> Renderer::createMaterial(MaterialType type)
{
  unsigned int offset = static_cast<unsigned int>(type);

  switch (type)
  {
  case MaterialType::DiffuseFlat:
    return std::make_shared<DiffuseFlatMaterial>(shaders[offset]);
    break;
  case MaterialType::DiffuseTextured:
    return std::make_shared<DiffuseTexturedMaterial>(shaders[offset]);
    break;
  case MaterialType::MaterialTypeCount:
    return std::make_shared<DiffuseFlatMaterial>(shaders[offset]);
    break;
  default:
    return nullptr;
    break;
  }
}

void Renderer::setAmbientLight(glm::vec3 color)
{
  ambientLight = color;
}