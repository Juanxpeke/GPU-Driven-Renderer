#include "terrain.h"

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include "../util/log.h"
#include "geoclipmap.h"

namespace Lotus
{

  Terrain::Terrain(std::shared_ptr<ProceduralDataGenerator> s, uint32_t levelsOfDetail, uint32_t resolution) :
    levels(levelsOfDetail),
    tileResolution(resolution),
    chunkGenerator(s)
  {
    meshes = GeoClipmap::generate(resolution);

    Lotus::TextureConfig textureConfig;
    textureConfig.format = Lotus::TextureFormat::RFloat;
    textureConfig.width = 256;
    textureConfig.height = 256;
    textureConfig.depth = chunkGenerator->getChunksAmount();

    heightmapTextures = std::make_shared<GPUTextureArray>(textureConfig);
    
    for (int x = 0; x < chunkGenerator->getChunksPerSide(); x++)
    {
      for (int y = 0; y < chunkGenerator->getChunksPerSide(); y++)
      {
        uint16_t layer = y * chunkGenerator->getChunksPerSide() + x;
        heightmapTextures->setLayerData(layer, chunkGenerator->getChunkData(x, y));
      }
    }

    clipmapProgram = ShaderProgram(shaderPath("terrain/clipmap.vert"), shaderPath("terrain/clipmap.frag"));

    rotationModels[0] = glm::mat4(1.0f);
    rotationModels[1] = glm::rotate(glm::mat4(1.0f), glm::radians( 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationModels[2] = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationModels[3] = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    debugColors[0] = glm::vec3(1.0, 1.0, 1.0);
    debugColors[1] = glm::vec3(0.0, 1.0, 1.0);
    debugColors[2] = glm::vec3(0.0, 1.0, 0.0);
    debugColors[3] = glm::vec3(0.0, 0.0, 1.0);
    debugColors[4] = glm::vec3(1.0, 0.0, 0.0);
  }

  void Terrain::setChunkGenerator(std::shared_ptr<ProceduralDataGenerator> s)
  {
    chunkGenerator = s;
  }

  void Terrain::render(const Camera& camera)
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    uint16_t quadSize = 1;
   
    glm::mat4 viewMatrix = camera.getViewMatrix();
    glm::mat4 projectionMatrix = camera.getProjectionMatrix();
    glm::vec3 cameraPosition = camera.getLocalTranslation();

    glUseProgram(clipmapProgram.getProgramID());

    glUniform1i(DataPerChunkSideBinding, 256);
    glUniform1i(ChunksPerSideBinding, chunkGenerator->getChunksPerSide());

    glUniformMatrix4fv(ViewBinding, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(ProjectionBinding, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniform1i(HeightmapTextureArrayBinding, HeightmapTextureUnit);

    glBindTextureUnit(HeightmapTextureUnit, heightmapTextures->getID());

    glm::vec3 movement = firstTimeCamera ? glm::vec3(0) : (cameraPosition - lastCameraPosition);

    if (firstTimeCamera) {
      lastCameraPosition = cameraPosition;
      firstTimeCamera = false;
      LOTUS_LOG_INFO("NOT FIRST TIME CAMERA");
    }

    if (movement.x > 256)
    {
      chunkGenerator->updateRight();

      for (int y = 0; y < chunkGenerator->getChunksPerSide(); y++)
      {
        uint16_t layer = y * chunkGenerator->getChunksPerSide() + chunkGenerator->getRight();
        heightmapTextures->setLayerData(layer, chunkGenerator->getChunkData(chunkGenerator->getRight(), y));
      }

      glUniform2i(ChunksDataOrigin, chunkGenerator->getDataOrigin().x, chunkGenerator->getDataOrigin().y);
      glUniform2i(ChunksOrigin, chunkGenerator->getLeft(), chunkGenerator->getUp());

      LOTUS_LOG_INFO("UPDATED RIGHT");

      lastCameraPosition.x = cameraPosition.x;
    }
    else if (movement.x < -256)
    {
      chunkGenerator->updateLeft();

      for (int y = 0; y < chunkGenerator->getChunksPerSide(); y++)
      {
        uint16_t layer = y * chunkGenerator->getChunksPerSide() + chunkGenerator->getLeft();
        heightmapTextures->setLayerData(layer, chunkGenerator->getChunkData(chunkGenerator->getLeft(), y));
      }

      glUniform2i(ChunksDataOrigin, chunkGenerator->getDataOrigin().x, chunkGenerator->getDataOrigin().y);
      glUniform2i(ChunksOrigin, chunkGenerator->getLeft(), chunkGenerator->getUp());

      LOTUS_LOG_INFO("UPDATED LEFT");

      lastCameraPosition.x = cameraPosition.x;
    }

    if (movement.z < -256)
    {
      chunkGenerator->updateUp();

      for (int x = 0; x < chunkGenerator->getChunksPerSide(); x++)
      {
        uint16_t layer = chunkGenerator->getUp() * chunkGenerator->getChunksPerSide() + x;
        heightmapTextures->setLayerData(layer, chunkGenerator->getChunkData(x, chunkGenerator->getUp()));
      }
      LOTUS_LOG_INFO("UPDATED TOP");

      glUniform2i(ChunksDataOrigin, chunkGenerator->getDataOrigin().x, chunkGenerator->getDataOrigin().y);
      glUniform2i(ChunksOrigin, chunkGenerator->getLeft(), chunkGenerator->getUp());

      lastCameraPosition.z = cameraPosition.z;
    }
    else if (movement.z > 256)
    {
      chunkGenerator->updateDown();

      for (int x = 0; x < chunkGenerator->getChunksPerSide(); x++)
      {
        uint16_t layer = chunkGenerator->getDown() * chunkGenerator->getChunksPerSide() + x;
        heightmapTextures->setLayerData(layer, chunkGenerator->getChunkData(x, chunkGenerator->getDown()));
      }

      glUniform2i(ChunksDataOrigin, chunkGenerator->getDataOrigin().x, chunkGenerator->getDataOrigin().y);
      glUniform2i(ChunksOrigin, chunkGenerator->getLeft(), chunkGenerator->getUp());

      LOTUS_LOG_INFO("UPDATED DOWN");

      lastCameraPosition.z = cameraPosition.z;
    }

    // Draw cross
    {
      float scale = 1.0;

      glm::vec2 snappedPos;
      snappedPos.x = std::floorf(cameraPosition.x);
      snappedPos.y = std::floorf(cameraPosition.z);

      glUniform1fv(LevelScaleBinding, 1, &scale);
      glUniformMatrix4fv(ModelBinding, 1, GL_FALSE, glm::value_ptr(rotationModels[0]));
      glUniform2fv(OffsetBinding, 1, glm::value_ptr(snappedPos));
      glUniform3fv(DebugColorBinding, 1, glm::value_ptr(debugColors[GeoClipmap::CROSS]));
      
      glBindVertexArray(meshes[GeoClipmap::CROSS]->getVertexArrayID());
      
      glDrawElements(GL_TRIANGLES, meshes[GeoClipmap::CROSS]->getIndicesCount(), GL_UNSIGNED_INT, nullptr);
    }

    for (uint32_t level = 0; level < levels; level++)
    {
      float scale = static_cast<float>(1 << level);

      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
      glEnable( GL_POLYGON_OFFSET_LINE );
      glPolygonOffset( -1, -1 );    
      
      glm::vec2 snappedPos;
      snappedPos.x = std::floorf(cameraPosition.x / scale) * scale;
      snappedPos.y = std::floorf(cameraPosition.z / scale) * scale;

      glm::vec2 tileSize((tileResolution * quadSize) << level);
      glm::vec2 levelOrigin = snappedPos - glm::vec2((tileResolution * quadSize) << (level + 1));

      glUniform1fv(LevelScaleBinding, 1, &scale);
      glUniformMatrix4fv(ModelBinding, 1, GL_FALSE, glm::value_ptr(rotationModels[0]));

      glBindVertexArray(meshes[GeoClipmap::TILE]->getVertexArrayID());

      for (int x = 0; x < 4; x++)
      {
        for (int y = 0; y < 4; y++)
        {
          if (level != 0 && (x == 1 || x == 2) && (y == 1 || y == 2))
          {
            continue;
          }

          glm::vec2 fill = glm::vec2((x >= 2 ? quadSize : 0), (y >= 2 ? quadSize : 0)) * scale;
          glm::vec2 tileOffset = levelOrigin + glm::vec2(x, y) * tileSize + fill;


          glUniform2fv(OffsetBinding, 1, glm::value_ptr(tileOffset));
          glUniform3fv(DebugColorBinding, 1, glm::value_ptr(debugColors[GeoClipmap::TILE]));

          glDrawElements(GL_TRIANGLES, meshes[GeoClipmap::TILE]->getIndicesCount(), GL_UNSIGNED_INT, nullptr);
        }
      }


      // Draw filler
      {
        glBindVertexArray(meshes[GeoClipmap::FILLER]->getVertexArrayID());

        glUniform2fv(OffsetBinding, 1, glm::value_ptr(snappedPos));
          glUniform3fv(DebugColorBinding, 1, glm::value_ptr(debugColors[GeoClipmap::FILLER]));

        glDrawElements(GL_TRIANGLES, meshes[GeoClipmap::FILLER]->getIndicesCount(), GL_UNSIGNED_INT, nullptr);
      }

      if (level == levels - 1)
      {
        continue;
      }

      float nextScale = scale * 2.0f;

      glm::vec2 nextSnappedPos;
      nextSnappedPos.x = std::floorf(cameraPosition.x / nextScale) * nextScale;
      nextSnappedPos.y = std::floorf(cameraPosition.z / nextScale) * nextScale;
      
      // Draw trim
      {
        glUniform1fv(LevelScaleBinding, 1, &scale);

        glm::vec2 tileCentre = snappedPos + glm::vec2(int(scale) >> 1);
        glm::vec2 d = glm::vec2(cameraPosition.x, cameraPosition.z) - nextSnappedPos;

        uint32_t rotationIndex = 0;
        rotationIndex |= (d.x >= scale ? 0 : 2);
        rotationIndex |= (d.y >= scale ? 0 : 1);

        glBindVertexArray(meshes[GeoClipmap::TRIM]->getVertexArrayID());

        glUniformMatrix4fv(ModelBinding, 1, GL_FALSE, glm::value_ptr(rotationModels[rotationIndex]));
        glUniform2fv(OffsetBinding, 1, glm::value_ptr(tileCentre));
        glUniform3fv(DebugColorBinding, 1, glm::value_ptr(debugColors[GeoClipmap::TRIM]));
        
        glDrawElements(GL_TRIANGLES, meshes[GeoClipmap::TRIM]->getIndicesCount(), GL_UNSIGNED_INT, nullptr);
      }
      // Draw seam
      {
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        glDisable( GL_POLYGON_OFFSET_LINE );
        
        glm::vec2 nextBase = nextSnappedPos - glm::vec2(static_cast<float>(tileResolution << (level + 1)));

        glBindVertexArray(meshes[GeoClipmap::SEAM]->getVertexArrayID());

        glUniformMatrix4fv(ModelBinding, 1, GL_FALSE, glm::value_ptr(rotationModels[0]));
        glUniform2fv(OffsetBinding, 1, glm::value_ptr(nextBase));
          glUniform3fv(DebugColorBinding, 1, glm::value_ptr(debugColors[GeoClipmap::SEAM]));

        glDrawElements(GL_TRIANGLES, meshes[GeoClipmap::SEAM]->getIndicesCount(), GL_UNSIGNED_INT, nullptr);
      }
    }

    glBindVertexArray(0);
  }

}