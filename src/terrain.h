#pragma once

#include <glm/glm.hpp>
#include <map>
#include <memory>
#include "mesh.h"

class TerrainParameters
{
   public:
    int   subdivisions   = 100;
    float tileSize       = 50.0f;
    int   renderDistance = 3;
    float textureScale   = 10.0f;
};

class Terrain
{
   public:
    Terrain(TerrainParameters params);

    void update(const glm::vec3& cameraPos);
    void render(const Shader& shader);

    void setParameters(TerrainParameters params);

   private:
    int   m_subdivisions;
    float m_tileSize;
    int   m_renderDistance;
    float m_textureScale;
    bool  m_generated = false;

    std::map<std::pair<int, int>, std::unique_ptr<GPUMesh>> m_tiles;
    int                                                     m_lastCameraTileX = -1;
    int                                                     m_lastCameraTileZ = -1;

    GPUMesh createTileMesh(int gridX, int gridZ);
    void    loadTiles(int centerTileX, int centerTileZ);
    void    unloadTiles(int centerTileX, int centerTileZ);
};