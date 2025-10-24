#include "terrain.h"
#include <cmath>
#include <iostream>

static void accumulateTangents(Mesh& mesh)
{
    for (auto& v : mesh.vertices)
    {
        v.tangent   = glm::vec3(0);
        v.bitangent = glm::vec3(0);
    }

    for (auto& tri : mesh.triangles)
    {
        Vertex& v0 = mesh.vertices[tri[0]];
        Vertex& v1 = mesh.vertices[tri[1]];
        Vertex& v2 = mesh.vertices[tri[2]];

        glm::vec3 p0 = v0.position, p1 = v1.position, p2 = v2.position;
        glm::vec2 uv0 = v0.texCoord, uv1 = v1.texCoord, uv2 = v2.texCoord;

        glm::vec3 dp1 = p1 - p0, dp2 = p2 - p0;
        glm::vec2 duv1 = uv1 - uv0, duv2 = uv2 - uv0;

        float     r = 1.0f / (duv1.x * duv2.y - duv1.y * duv2.x + 1e-20f);
        glm::vec3 T = (dp1 * duv2.y - dp2 * duv1.y) * r;
        glm::vec3 B = (dp2 * duv1.x - dp1 * duv2.x) * r;

        v0.tangent += T;
        v1.tangent += T;
        v2.tangent += T;
        v0.bitangent += B;
        v1.bitangent += B;
        v2.bitangent += B;
    }
    for (auto& v : mesh.vertices)
    {
        glm::vec3 n = glm::normalize(v.normal);
        glm::vec3 t = glm::normalize(v.tangent - n * glm::dot(n, v.tangent));
        glm::vec3 b = glm::normalize(glm::cross(n, t));
        v.tangent   = t;
        v.bitangent = b;
    }
}

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
    accumulateTangents(mesh);

    mesh.material.kd        = glm::vec3(0.8f, 0.8f, 0.8f);
    mesh.material.ks        = glm::vec3(0.1f, 0.1f, 0.1f);
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