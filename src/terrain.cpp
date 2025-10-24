#include "terrain.h"
#include <cmath>
#include <iostream>

Terrain::Terrain(TerrainParameters params)
    : m_subdivisions(params.subdivisions),
      m_tileSize(params.tileSize),
      m_renderDistance(params.renderDistance),
      m_textureScale(params.textureScale)

{
}

void Terrain::update(const glm::vec3& cameraPos)
{
    // Calculate indices of a tile below camera
    int cameraTileX = int(floor(cameraPos.x / m_tileSize));
    int cameraTileZ = int(floor(cameraPos.z / m_tileSize));

    // load/unload tiles if the camera moved to another tile (or Regenerate Terrain pressed)
    if (!m_generated || m_tiles.empty() || cameraTileX != m_lastCameraTileX || cameraTileZ != m_lastCameraTileZ)
    {
        loadTiles(cameraTileX, cameraTileZ);
        unloadTiles(cameraTileX, cameraTileZ);
        m_lastCameraTileX = cameraTileX;
        m_lastCameraTileZ = cameraTileZ;
        m_generated       = true;
    }
}

void Terrain::render(const Shader& shader)
{
    for (auto& pair : m_tiles)
    {
        pair.second->draw(shader);
    }
}

void Terrain::setParameters(TerrainParameters params)
{
    m_subdivisions   = params.subdivisions;
    m_tileSize       = params.tileSize;
    m_renderDistance = params.renderDistance;
    m_textureScale   = params.textureScale;
    m_generated      = false;
    m_tiles.clear();
}

GPUMesh Terrain::createTileMesh(int gridX, int gridZ)
{
    Mesh mesh;

    const float step    = m_tileSize / (m_subdivisions - 1);
    const float offsetX = gridX * m_tileSize;
    const float offsetZ = gridZ * m_tileSize;

    // Create vertices
    for (int z = 0; z < m_subdivisions; z++)
    {
        for (int x = 0; x < m_subdivisions; x++)
        {
            Vertex v;
            float  worldX = offsetX + x * step;
            float  worldZ = offsetZ + z * step;
            v.position    = glm::vec3(worldX, 0.0f, worldZ);
            v.normal      = glm::vec3(0.0f, 1.0f, 0.0f);

            v.texCoord = glm::vec2(worldX / m_textureScale, worldZ / m_textureScale);

            mesh.vertices.push_back(v);
        }
    }

    // Create triangles
    for (int z = 0; z < m_subdivisions - 1; z++)
    {
        for (int x = 0; x < m_subdivisions - 1; x++)
        {
            int topLeft     = z * m_subdivisions + x;
            int topRight    = topLeft + 1;
            int bottomLeft  = (z + 1) * m_subdivisions + x;
            int bottomRight = bottomLeft + 1;

            mesh.triangles.emplace_back(topLeft, bottomLeft, topRight);
            mesh.triangles.emplace_back(topRight, bottomLeft, bottomRight);
        }
    }

    mesh.material.kd        = glm::vec3(0.8f, 0.8f, 0.8f);
    mesh.material.ks        = glm::vec3(0.04f, 0.04f, 0.04f);
    mesh.material.shininess = 8.0f;
    return GPUMesh(mesh);
}

void Terrain::loadTiles(int centerTileX, int centerTileZ)
{
    for (int z = centerTileZ - m_renderDistance; z <= centerTileZ + m_renderDistance; z++)
    {
        for (int x = centerTileX - m_renderDistance; x <= centerTileX + m_renderDistance; x++)
        {
            auto key = std::make_pair(x, z);
            if (!m_tiles.contains(key))
            {
                // Create a new tile if it is not yet created and in the visible distance
                m_tiles[key] = std::make_unique<GPUMesh>(createTileMesh(x, z));
            }
        }
    }
}

void Terrain::unloadTiles(int centerTileX, int centerTileZ)
{
    int maxDist = m_renderDistance + 1;

    auto it = m_tiles.begin();
    while (it != m_tiles.end())
    {
        int dx = abs(it->first.first - centerTileX);
        int dz = abs(it->first.second - centerTileZ);

        if (dx > maxDist || dz > maxDist)
        {
            it = m_tiles.erase(it);
        }
        else
        {
            ++it;
        }
    }
}