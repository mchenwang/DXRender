#include "ModelLoader.h"
#include "PlyHelper.h"
#include "ObjHelper.h"
#include <cassert>

std::unique_ptr<ModelLoader> ModelLoader::CreateModelLoader(ModelType type)
{
    // path.substr(path.size() - 4, 4) != ".obj"
    switch (type)
    {
    case ModelType::PLY:
        return std::make_unique<PLYModelLoader>();
    //TODO
    case ModelType::OBJ:
        return std::make_unique<OBJModelLoader>();
    default:
        throw std::exception("Unimplemented type");
    }
    return nullptr;
}

std::vector<std::array<double, 3>> ModelLoader::GetPositions()
{
    return m_positions;
}
std::vector<std::array<double, 3>> ModelLoader::GetNormals()
{
    return m_normals;
}
std::vector<uint32_t> ModelLoader::GetIndicies()
{
    return m_indicies;
}

void ModelLoader::SetPositions(std::vector<std::array<double, 3>>& positions)
{
    m_positions = positions;
}

// cut a polygon to several triangles
// Note that the face list generates triangles in the order of a TRIANGLE FAN, not a TRIANGLE STRIP. In the example above, the first face
//   4 0 1 2 3
// Is composed of the triangles 0,1,2 and 0,2,3 and not 0,1,2 and 1,2,3.
static void CutPolygon(std::vector<uint32_t>& polygon, std::vector<std::array<uint32_t, 3>>& triangles)
{
    uint32_t i0 = 0;
    // uint32_t i1 = 1;
    uint32_t i2 = 2;
    for (; i2 < polygon.size(); i2++)
    {
        uint32_t i1 = i2 - 1;
        triangles.emplace_back(std::array<uint32_t, 3>{polygon[i0], polygon[i1], polygon[i2]});
    }
    return ;
}

void ModelLoader::SetIndicies(std::vector<std::vector<uint32_t>>& faces)
{
    std::vector<uint32_t> indicies;
    for(auto& face: faces)
    {
        assert(face.size() >= 3 && "model format error.");
        std::vector<std::array<uint32_t, 3>> triangles;
        CutPolygon(face, triangles);
        for (auto& tri: triangles)
        {
            indicies.push_back(tri[0]);
            indicies.push_back(tri[1]);
            indicies.push_back(tri[2]);
        }
    }
    
    m_indicies.swap(indicies);
}

void ModelLoader::Reconstruct()
{
    if (!m_initialized || m_positions.size() == 0) return;

    float maxX, minX, maxY, minY, maxZ, minZ;
    maxX = minX = m_positions[0][0];
    maxY = minY = m_positions[0][1];
    maxZ = minZ = m_positions[0][2];

    for (int i = 1; i < m_positions.size(); ++i)
    {
        if (m_positions[i][0] > maxX) maxX = m_positions[i][0];
        if (m_positions[i][0] < minX) minX = m_positions[i][0];
        if (m_positions[i][1] > maxY) maxY = m_positions[i][1];
        if (m_positions[i][1] < minY) minY = m_positions[i][1];
        if (m_positions[i][2] > maxZ) maxZ = m_positions[i][2];
        if (m_positions[i][2] < minZ) minZ = m_positions[i][2];
    }

    float offsetX = (maxX + minX) / 2.;
    float offsetY = (maxY + minY) / 2.;
    float offsetZ = (maxZ + minZ) / 2.;

    float rangeX = maxX - minX;
    float rangeY = maxY - minY;
    float rangeZ = maxZ - minZ;
    float range = std::max(rangeX, std::max(rangeY, rangeZ)) / 2.;

    for (int i = 0; i < m_positions.size(); ++i)
    {
        m_positions[i][0] = (m_positions[i][0] - offsetX) / range;
        m_positions[i][1] = (m_positions[i][1] - offsetY) / range;
        m_positions[i][2] = (m_positions[i][2] - offsetZ) / range;
    }
}

#pragma region PLY
void PLYModelLoader::LoadFromFile(std::wstring& filePath)
{
    happly::PLYData plyIn(Util::ToByteString(filePath));
    SetPositions(plyIn.getVertexPositions());
    SetIndicies(plyIn.getFaceIndices<uint32_t>());

    m_initialized = true;
}
#pragma endregion

#pragma region OBJ
void OBJModelLoader::LoadFromFile(std::wstring& filePath)
{
    ObjHelper::ObjLoader objIn(Util::ToByteString(filePath));
    SetPositions(objIn.GetverticesPosition());

    auto normals = objIn.GetVerticesNormal();
    if (normals.size() == 0)
    {
        SetIndicies(objIn.GetFacesVertexIndex());
    }
    else
    {
        auto facesNormalIndex = objIn.GetFacesVertexNormalIndex();
        auto facesVertexIndex = objIn.GetFacesVertexIndex();
        SetIndiciesAndNormals(facesVertexIndex, facesNormalIndex, normals);
    }

}

namespace
{
    struct VertexNormalIndex
    {
        uint32_t vertex;
        int normal; // -1 特判
    };

    std::array<double, 3>& operator+=(std::array<double, 3>& a, std::array<double, 3>& b)
    {
        a[0] += b[0];
        a[1] += b[1];
        a[2] += b[2];
        return a;
    }
}

void OBJModelLoader::SetIndiciesAndNormals(std::vector<std::vector<uint32_t>>& facesVertex,
        std::vector<std::vector<uint32_t>>& facesNormal,
        std::vector<std::array<double, 3>>& normals)
{
    m_normals.resize(m_positions.size());
    std::fill(m_normals.begin(), m_normals.end(), std::array<double, 3>{0., 0., 0.});

    for (int i = 0; i < facesVertex.size(); i++)
    {
        assert(facesVertex[i].size() >= 3 && "model format error.");
        for (int j = 0; j < facesVertex[i].size(); j++)
        {
            if (facesNormal[i].size() > j)
            {
                m_normals[facesVertex[i][j]] += normals[facesNormal[i][j]];
            }
            
        }

        std::vector<std::array<uint32_t, 3>> triangles;
        CutPolygon(facesVertex[i], triangles);
        for (auto& tri: triangles)
        {
            m_indicies.push_back(tri[0]);
            m_indicies.push_back(tri[1]);
            m_indicies.push_back(tri[2]);
        }
    }
}
#pragma endregion
