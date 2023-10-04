#pragma once

/*
===================================
Placeholder script, don't modify it
===================================
*/

#include <iostream>
#include <string>
#include <filesystem>

static const std::string directoryPath = "C:/Users/juani/Desktop/Computer Graphics/Bachelor Thesis/OpenGL-Rendering-Engine/";

static std::filesystem::path assetPath(const std::string& assetFilename) {
  return directoryPath + "assets/" + assetFilename;
}

static std::filesystem::path shaderPath(const std::string& shaderFilename)
{
  return directoryPath + "renderer/shaders/" + shaderFilename;
}