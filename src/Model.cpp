#include "Model.h"
#include "path.h"
#include "common/ModelLoader.h"

std::wstring Model::GetModelFullPath(std::wstring model_name)
{
    return std::wstring(model_path) + model_name;
}

Model::Model(std::wstring model_name, ModelType type, bool reconstruct) noexcept
{
    auto loader = ModelLoader::CreateModelLoader(type);
    loader->LoadFromFile(Model::GetModelFullPath(model_name));
    if (reconstruct)
    {
        loader->Reconstruct();
    }
    
    auto positions = loader->GetPositions();
    m_vertices = std::vector<Vertex>(positions.size());
    for (int i = 0; i < m_vertices.size(); ++i)
    {
        m_vertices[i].position = XMFLOAT3(positions[i][0], positions[i][1], positions[i][2]);
        m_vertices[i].normal   = XMFLOAT3(0.f, 0.f, 0.f);
    }

    auto indicies = loader->GetIndicies();
    m_indicies.swap(indicies);

    auto normals = loader->GetNormals();
    if (normals.size() == 0) CalculateVertexNormal();
    else
    {
        for (int i = 0; i < m_vertices.size(); ++i)
        {
            auto ret = XMVector3Normalize(XMVectorSet(normals[i][0], normals[i][1], normals[i][2], 0.f));
            XMStoreFloat3(&m_vertices[i].normal, ret);
        }
    }
}

static DirectX::XMVECTOR operator-(const DirectX::XMFLOAT3& A, const DirectX::XMFLOAT3& B)
{
    return DirectX::XMVectorSet(A.x - B.x, A.y - B.y, A.z - B.z, 0.f);
}
static DirectX::XMFLOAT3 operator+=(DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

void Model::CalculateVertexNormal()
{
    for (int i = 0; i + 2 < m_indicies.size(); i += 3)
    {
        auto A = m_vertices[m_indicies[i    ]].position;
        auto B = m_vertices[m_indicies[i + 1]].position;
        auto C = m_vertices[m_indicies[i + 2]].position;
        auto AB = B - A;
        auto AC = C - A;

        DirectX::XMFLOAT3 normal;
        XMStoreFloat3(&normal, XMVector3Cross(AB, AC));

        m_vertices[m_indicies[i    ]].normal += normal;
        m_vertices[m_indicies[i + 1]].normal += normal;
        m_vertices[m_indicies[i + 2]].normal += normal;
    }

    for (int i = 0; i < m_vertices.size(); ++i)
    {
        auto ret = XMVector3Normalize(XMLoadFloat3(&m_vertices[i].normal));
        XMStoreFloat3(&m_vertices[i].normal, ret);
    }
}

std::vector<Vertex> Model::GetVertices() const
{
    return m_vertices;
}
std::vector<uint32_t> Model::GetIndicies() const
{
    return m_indicies;
}
uint32_t Model::GetVerticesNum() const
{
    return m_vertices.size();
}
uint32_t Model::GetIndiciesNum() const
{
    return m_indicies.size();
}