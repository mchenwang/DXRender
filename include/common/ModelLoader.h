#ifndef __MODELLOADER_H__
#define __MODELLOADER_H__

#include <vector>
#include <string>
#include <memory>
#include <array>

enum class ModelType: uint32_t
{
    PLY,
    OBJ // TODO
};

class ModelLoader
{
protected:
    std::vector<std::array<double, 3>> m_positions;
    std::vector<std::array<double, 3>> m_normals;
    std::vector<uint32_t> m_indicies;
    bool m_initialized = false;

    ModelLoader() = default;

    virtual void SetPositions(std::vector<std::array<double, 3>>& positions);
    virtual void SetIndicies(std::vector<std::vector<uint32_t>>& faces);

public:
    ~ModelLoader() = default;
    static std::unique_ptr<ModelLoader> CreateModelLoader(ModelType);
    
    // 将模型移动放缩到 [-1,1]^3 的空间内
    void Reconstruct();
    virtual void LoadFromFile(std::wstring& filePath) = 0;

    std::vector<std::array<double, 3>> GetPositions();
    // 未归一化的顶点法线
    std::vector<std::array<double, 3>> GetNormals();
    std::vector<uint32_t> GetIndicies();
};

class PLYModelLoader : public ModelLoader
{
public:
    PLYModelLoader() = default;
    ~PLYModelLoader() = default;
    void LoadFromFile(std::wstring& filePath) override;
};

// TODO
class OBJModelLoader : public ModelLoader
{
    void SetIndiciesAndNormals(std::vector<std::vector<uint32_t>>& facesVertex,
         std::vector<std::vector<uint32_t>>& facesNormal,
         std::vector<std::array<double, 3>>& normals); // vertex/texture/normal
public:
    OBJModelLoader() = default;
    ~OBJModelLoader() = default;
    void LoadFromFile(std::wstring& filePath) override;
};
#endif