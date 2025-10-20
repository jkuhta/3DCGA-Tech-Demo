#include "terrain.h"
#include <cmath>
#include <iostream>

Terrain::Terrain(TerrainParameters params)
    : m_subdivisions(params.subdivisions), m_tileSize(params.tileSize), m_renderDistance(params.renderDistance)
{
}

void Terrain::update(const glm::vec3& cameraPos)
{
    int cameraX = int(floor(cameraPos.x / m_tileSize));
    int cameraZ = int(floor(cameraPos.z / m_tileSize));

    std::cout << "m_generated: " << m_generated << m_renderDistance << std::endl;
    if (!m_generated || m_tiles.empty() || cameraX != m_lastCameraX || cameraZ != m_lastCameraZ)
    {
        loadTiles(cameraX, cameraZ);
        unloadTiles(cameraX, cameraZ);
        m_lastCameraX = cameraX;
        m_lastCameraZ = cameraZ;
        m_generated   = true;
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
            v.position = glm::vec3(offsetX + x * step, 0.0f, offsetZ + z * step);
            v.normal   = glm::vec3(0.0f, 1.0f, 0.0f);
            v.texCoord = glm::vec2(float(x) / (m_subdivisions - 1), float(z) / (m_subdivisions - 1));
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

    mesh.material.kd = glm::vec3(0.5f, 0.7f, 0.3f);
    return GPUMesh(mesh);
}

void Terrain::loadTiles(int centerX, int centerZ)
{
    for (int z = centerZ - m_renderDistance; z <= centerZ + m_renderDistance; z++)
    {
        for (int x = centerX - m_renderDistance; x <= centerX + m_renderDistance; x++)
        {
            auto key = std::make_pair(x, z);
            if (m_tiles.find(key) == m_tiles.end())
            {
                m_tiles[key] = std::make_unique<GPUMesh>(createTileMesh(x, z));
                std::cout << "Created tile at (" << x << ", " << z << ")" << std::endl;  // Add debug
            }
        }
    }
    std::cout << "Total tiles: " << m_tiles.size() << std::endl;
}

void Terrain::unloadTiles(int centerX, int centerZ)
{
    int maxDist = m_renderDistance + 1;

    auto it = m_tiles.begin();
    while (it != m_tiles.end())
    {
        int dx = abs(it->first.first - centerX);
        int dz = abs(it->first.second - centerZ);

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