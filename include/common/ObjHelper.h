#ifndef __OBJHELPER_H__
#define __OBJHELPER_H__

#include <fstream>
#include <array>
#include <vector>
#include "Utility.h"

namespace ObjHelper
{
    using std::ifstream;
    using std::array;
    using std::vector;
    using std::string;

    class ObjLoader
    {
        vector<array<double, 3>> m_positions;
        vector<array<double, 3>> m_normals;
        vector<array<double, 3>> m_uvws;

        vector<vector<uint32_t>> m_facesVertexIndex;
        vector<vector<uint32_t>> m_facesVertexNormal;
        vector<vector<uint32_t>> m_facesVertexUVW;
        void Clear()
        {
            m_positions.clear();
            m_normals.clear();
            m_uvws.clear();
        }
    public:
        ObjLoader() = delete;
        ~ObjLoader() = default;
        ObjLoader(string& filePath)
        {
            LoadFromFile(filePath);
        }

        void LoadFromFile(string& filePath)
        {
            Clear();
            ifstream in;
            in.open(filePath, ifstream::in);
            if (in.fail())
            {
                throw std::exception("obj file cannot be opened.");
            }

            string line;
            bool fail = false;
            while (!in.eof())
            {
                std::getline(in, line);
                vector<string> substr;
                Util::Split(line, substr, ' ');
                if (substr.size() == 0) continue;
                if (substr[0].compare("v") == 0)
                {
                    substr = Util::Filter(substr, "");
                    if (substr.size() != 4)
                    {
                        fail = true;
                        break;
                    }

                    m_positions.emplace_back(array<double, 3>{std::stod(substr[1]), std::stod(substr[2]), std::stod(substr[3])});
                }
                else if (substr[0].compare("vn") == 0)
                {
                    substr = Util::Filter(substr, "");
                    if (substr.size() != 4)
                    {
                        fail = true;
                        break;
                    }

                    m_normals.emplace_back(array<double, 3>{std::stod(substr[1]), std::stod(substr[2]), std::stod(substr[3])});
                }
                else if (substr[0].compare("vt") == 0)
                {
                    substr = Util::Filter(substr, "");
                    if (substr.size() == 3)
                    {
                        m_uvws.emplace_back(array<double, 3>{std::stod(substr[1]), std::stod(substr[2]), 0.});
                    }
                    else if (substr.size() == 4)
                    {
                        m_uvws.emplace_back(array<double, 3>{std::stod(substr[1]), std::stod(substr[2]), std::stod(substr[3])});
                    }
                    else
                    {
                        fail = true;
                        break;
                    }
                }
                else if (substr[0].compare("f") == 0)
                {
                    if (substr.size() < 3)
                    {
                        fail = true;
                        break;
                    }

                    vector<uint32_t> faceVertexIndex;
                    vector<uint32_t> faceVertexNormal;
                    vector<uint32_t> faceVertexUVW;
                    for (int i = 1; i < substr.size(); i++)
                    {
                        vector<string> vertexInfo;
                        Util::Split(substr[i], vertexInfo, '/');
                        if (vertexInfo.size() == 1)
                        {
                            faceVertexIndex.emplace_back(std::stoi(vertexInfo[0]) - 1);
                        }
                        else if (vertexInfo.size() == 2)
                        {
                            faceVertexIndex.emplace_back(std::stoi(vertexInfo[0]) - 1);
                            faceVertexUVW.emplace_back(std::stoi(vertexInfo[1]) - 1);
                        }
                        else if (vertexInfo.size() == 3)
                        {
                            faceVertexIndex.emplace_back(std::stoi(vertexInfo[0]) - 1);
                            if (vertexInfo[1].size() > 0)
                            {
                                faceVertexUVW.emplace_back(std::stoi(vertexInfo[1]) - 1);
                            }
                            faceVertexNormal.emplace_back(std::stoi(vertexInfo[2]) - 1);
                        }
                        else
                        {
                            fail = true;
                            break;
                        }
                    }

                    if (fail) break;
                    m_facesVertexIndex.emplace_back(faceVertexIndex);
                    m_facesVertexUVW.emplace_back(faceVertexUVW);
                    m_facesVertexNormal.emplace_back(faceVertexNormal);
                }
            }

            if (fail)
            {
                throw std::exception("obj format error.");
            }
        }
    
        auto GetverticesPosition() const
        {
            return m_positions;
        }
        auto GetVerticesNormal() const
        {
            return m_normals;
        }
        auto GetVerticesUVW() const
        {
            return m_uvws;
        }
        auto GetFacesVertexIndex() const
        {
            return m_facesVertexNormal;
        }
        auto GetFacesVertexNormalIndex() const
        {
            return m_facesVertexNormal;
        }
        auto GetFacesVertexUVWIndex() const
        {
            return m_facesVertexUVW;
        }
    };

}
#endif