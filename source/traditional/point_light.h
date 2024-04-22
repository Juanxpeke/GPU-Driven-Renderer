#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "../scene/node_3d.h"

class PointLight : public Node3D
{
public:

  PointLight(
      const glm::vec3& lightColor = glm::vec3(1.0f),
      float radius = 10.0f) :
    maxRadius(radius), color(lightColor) {};


  const glm::vec3& getLightColor() const { return color; }
  float getMaxRadius() const { return maxRadius; }

  void setLightColor(const glm::vec3& lightColor) { color = lightColor; }
  void setMaxRadius(float radius) { maxRadius = radius; }

private:
  glm::vec3 color;
  float maxRadius;
};