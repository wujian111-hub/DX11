#pragma once
//***************************************************************************************
// Geometry.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// Generate common geometry meshes.
//***************************************************************************************

#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <type_traits>
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include "Vertex.h"

namespace Geometry
{
    // Mesh data
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    struct MeshData
    {
        std::vector<VertexType> vertexVec; // vertices
        std::vector<IndexType> indexVec;   // indices

        MeshData()
        {
            static_assert(sizeof(IndexType) == 2 || sizeof(IndexType) == 4, "The size of IndexType must be 2 bytes or 4 bytes!");
            static_assert(std::is_unsigned<IndexType>::value, "IndexType must be unsigned integer!");
        }
    };

    // Create sphere mesh data.
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateSphere(float radius = 1.0f, UINT levels = 20, UINT slices = 20,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // Create box mesh data
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateBox(float width = 2.0f, float height = 2.0f, float depth = 2.0f,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建圆柱体网格数据，slices越大，精度越高。
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateCylinder(float radius = 1.0f, float height = 2.0f, UINT slices = 20, UINT stacks = 10,
        float texU = 1.0f, float texV = 1.0f, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建只有圆柱体侧面的网格数据，slices越大，精度越高
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateCylinderNoCap(float radius = 1.0f, float height = 2.0f, UINT slices = 20, UINT stacks = 10,
        float texU = 1.0f, float texV = 1.0f, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建圆锥体网格数据，slices越大，精度越高。
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateCone(float radius = 1.0f, float height = 2.0f, UINT slices = 20,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建只有圆锥体侧面网格数据，slices越大，精度越高。
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateConeNoCap(float radius = 1.0f, float height = 2.0f, UINT slices = 20,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建一个指定NDC屏幕区域的面(默认全屏)
    template<class VertexType = VertexPosTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> Create2DShow(const DirectX::XMFLOAT2& center, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
    template<class VertexType = VertexPosTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> Create2DShow(float centerX = 0.0f, float centerY = 0.0f, float scaleX = 1.0f, float scaleY = 1.0f, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建一个平面
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreatePlane(const DirectX::XMFLOAT2& planeSize,
        const DirectX::XMFLOAT2& maxTexCoord = { 1.0f, 1.0f }, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreatePlane(float width = 10.0f, float depth = 10.0f, float texU = 1.0f, float texV = 1.0f,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建一个地形
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateTerrain(const DirectX::XMFLOAT2& terrainSize,
        const DirectX::XMUINT2& slices = { 10, 10 }, const DirectX::XMFLOAT2& maxTexCoord = { 1.0f, 1.0f },
        const std::function<float(float, float)>& heightFunc = [](float x, float z) { return 0.0f; },
        const std::function<DirectX::XMFLOAT3(float, float)>& normalFunc = [](float x, float z) { return DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f); },
        const std::function<DirectX::XMFLOAT4(float, float)>& colorFunc = [](float x, float z) { return DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); });
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateTerrain(float width = 10.0f, float depth = 10.0f,
        UINT slicesX = 10, UINT slicesZ = 10, float texU = 1.0f, float texV = 1.0f,
        const std::function<float(float, float)>& heightFunc = [](float x, float z) { return 0.0f; },
        const std::function<DirectX::XMFLOAT3(float, float)>& normalFunc = [](float x, float z) { return DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f); },
        const std::function<DirectX::XMFLOAT4(float, float)>& colorFunc = [](float x, float z) { return DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); });
}


namespace Geometry
{
    namespace Internal
    {
        struct VertexData
        {
            DirectX::XMFLOAT3 pos;
            DirectX::XMFLOAT3 normal;
            DirectX::XMFLOAT4 tangent;
            DirectX::XMFLOAT4 color;
            DirectX::XMFLOAT2 tex;
        };

        template<class VertexType>
        inline void InsertVertexElement(VertexType& vertexDst, const VertexData& vertexSrc)
        {
            static std::string semanticName;
            static const std::map<std::string, std::pair<size_t, size_t>> semanticSizeMap = {
                {"POSITION", std::pair<size_t, size_t>(0, 12)},
                {"NORMAL", std::pair<size_t, size_t>(12, 24)},
                {"TANGENT", std::pair<size_t, size_t>(24, 40)},
                {"COLOR", std::pair<size_t, size_t>(40, 56)},
                {"TEXCOORD", std::pair<size_t, size_t>(56, 64)}
            };

            for (size_t i = 0; i < ARRAYSIZE(VertexType::inputLayout); i++)
            {
                semanticName = VertexType::inputLayout[i].SemanticName;
                const auto& range = semanticSizeMap.at(semanticName);
                memcpy_s(reinterpret_cast<char*>(&vertexDst) + VertexType::inputLayout[i].AlignedByteOffset,
                    range.second - range.first,
                    reinterpret_cast<const char*>(&vertexSrc) + range.first,
                    range.second - range.first);
            }
        }
    }

    template<class VertexType, class IndexType>
    inline MeshData<VertexType, IndexType> CreateSphere(float radius, UINT levels, UINT slices, const DirectX::XMFLOAT4& color)
    {
        using namespace DirectX;

        MeshData<VertexType, IndexType> meshData;
        UINT vertexCount = 2 + (levels - 1) * (slices + 1);
        UINT indexCount = 6 * (levels - 1) * slices;
        meshData.vertexVec.resize(vertexCount);
        meshData.indexVec.resize(indexCount);

        Internal::VertexData vertexData;
        IndexType vIndex = 0, iIndex = 0;

        float phi = 0.0f, theta = 0.0f;
        float per_phi = XM_PI / levels;
        float per_theta = XM_2PI / slices;
        float x, y, z;

        vertexData = { XMFLOAT3(0.0f, radius, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.0f, 0.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        for (UINT i = 1; i < levels; ++i)
        {
            phi = per_phi * i;
            for (UINT j = 0; j <= slices; ++j)
            {
                theta = per_theta * j;
                x = radius * sinf(phi) * cosf(theta);
                y = radius * cosf(phi);
                z = radius * sinf(phi) * sinf(theta);

                XMFLOAT3 pos = XMFLOAT3(x, y, z), normal;
                XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&pos)));

                vertexData = { pos, normal, XMFLOAT4(-sinf(theta), 0.0f, cosf(theta), 1.0f), color, XMFLOAT2(theta / XM_2PI, phi / XM_PI) };
                Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
            }
        }

        vertexData = { XMFLOAT3(0.0f, -radius, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f),
            XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.0f, 1.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        if (levels > 1)
        {
            for (UINT j = 1; j <= slices; ++j)
            {
                meshData.indexVec[iIndex++] = 0;
                meshData.indexVec[iIndex++] = j % (slices + 1) + 1;
                meshData.indexVec[iIndex++] = j;
            }
        }

        for (UINT i = 1; i < levels - 1; ++i)
        {
            for (UINT j = 1; j <= slices; ++j)
            {
                meshData.indexVec[iIndex++] = (i - 1) * (slices + 1) + j;
                meshData.indexVec[iIndex++] = (i - 1) * (slices + 1) + j % (slices + 1) + 1;
                meshData.indexVec[iIndex++] = i * (slices + 1) + j % (slices + 1) + 1;

                meshData.indexVec[iIndex++] = i * (slices + 1) + j % (slices + 1) + 1;
                meshData.indexVec[iIndex++] = i * (slices + 1) + j;
                meshData.indexVec[iIndex++] = (i - 1) * (slices + 1) + j;
            }
        }

        if (levels > 1)
        {
            for (UINT j = 1; j <= slices; ++j)
            {
                meshData.indexVec[iIndex++] = (levels - 2) * (slices + 1) + j;
                meshData.indexVec[iIndex++] = (levels - 2) * (slices + 1) + j % (slices + 1) + 1;
                meshData.indexVec[iIndex++] = (levels - 1) * (slices + 1) + 1;
            }
        }

        return meshData;
    }
}

#endif


