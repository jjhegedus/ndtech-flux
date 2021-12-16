#pragma once

#include "pch.h"

namespace ndtech {

    // Constant buffer used to send hologram position transform to the shader pipeline.
    struct ModelConstantBuffer
    {
        DirectX::XMFLOAT4X4 model;
        DirectX::XMFLOAT4   hologramColorFadeMultiplier;
    };

    struct FrameConstantBuffer {

        DirectX::XMFLOAT4   eyePosition;
    };

    // Used to send per-vertex data to the vertex shader.
    struct VertexPosition
    {
        DirectX::XMFLOAT3 pos;
    };

    // Used to send per-vertex data to the vertex shader.
    struct VertexPositionColor
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 color;
    };

    // Used to send per-vertex data to the vertex shader.
    struct VertexPositionColorSize
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 color;
        DirectX::XMFLOAT3 size;
    };

    // Used to send per-vertex data to the vertex shader.
    struct VertexPositionColorTex
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 color;
        DirectX::XMFLOAT2 texCoord;
    };

    // Used to send per-vertex data to the vertex shader.
    struct VertexPositionNormalTex
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 texCoord;
    };

    static std::map<std::wstring, bool>            requiresModelConstantBufferMap = {
        { L"ms-appx:///BillboardVertexShader.cso", true },
    { L"ms-appx:///BillboardGeometryShader.cso", true },
    { L"ms-appx:///BillboardPixelShader.cso", false },
    { L"ms-appx:///SpinningCubeVertexShader.cso", true },
    { L"ms-appx:///SpinningCubePixelShader.cso", false },
    { L"ms-appx:///SpinningCubeGeometryShader.cso", false },
    { L"ms-appx:///SpinningCubeVprtVertexShader.cso", true },
    { L"ms-appx:///BubbleVertexShader.cso", true },
    { L"ms-appx:///BubblePixelShader.cso", false },
    { L"ms-appx:///BubbleGeometryShader.cso", false },
    { L"ms-appx:///TextBillboardVertexShader.cso", true },
    { L"ms-appx:///TextBillboardVprtVertexShader.cso", true },
    { L"ms-appx:///UseDistanceFieldPixelShader.cso", true },
    { L"ms-appx:///TextBillboardGeometryShader.cso", false },
    };
    static bool RequiresModelConstantBuffer(std::wstring fileName) {
        auto it = requiresModelConstantBufferMap.find(fileName);
        return (*it).second;
    }

    static std::map<std::wstring, bool>            requiresFrameConstantBufferMap = {
        { L"ms-appx:///BillboardVertexShader.cso", false },
    { L"ms-appx:///BillboardGeometryShader.cso", true },
    { L"ms-appx:///BillboardPixelShader.cso", false },
    { L"ms-appx:///SpinningCubeVertexShader.cso", false },
    { L"ms-appx:///SpinningCubePixelShader.cso", false },
    { L"ms-appx:///SpinningCubeGeometryShader.cso", false },
    { L"ms-appx:///SpinningCubeVprtVertexShader.cso", false },
    { L"ms-appx:///BubbleVertexShader.cso", false },
    { L"ms-appx:///BubblePixelShader.cso", false },
    { L"ms-appx:///BubbleGeometryShader.cso", false },
    { L"ms-appx:///BubbleVprtVertexShader.cso", false },
    { L"ms-appx:///TextBillboardVertexShader.cso", false },
    { L"ms-appx:///TextBillboardVprtVertexShader.cso", false },
    { L"ms-appx:///UseDistanceFieldPixelShader.cso", true },
    { L"ms-appx:///TextBillboardGeometryShader.cso", false },
    };
    static bool RequiresFrameConstantBuffer(std::wstring fileName) {
        auto it = requiresFrameConstantBufferMap.find(fileName);
        return (*it).second;
    }

    static std::map<std::wstring, bool>            requiresViewProjectionBuffer = {
        { L"ms-appx:///BillboardVertexShader.cso", true },
    { L"ms-appx:///BillboardGeometryShader.cso", true },
    { L"ms-appx:///BillboardPixelShader.cso", false },
    { L"ms-appx:///SpinningCubeVertexShader.cso", true },
    { L"ms-appx:///SpinningCubePixelShader.cso", false },
    { L"ms-appx:///SpinningCubeGeometryShader.cso", false },
    { L"ms-appx:///SpinningCubeVprtVertexShader.cso", true },
    { L"ms-appx:///BubbleVertexShader.cso", true },
    { L"ms-appx:///BubblePixelShader.cso", false },
    { L"ms-appx:///BubbleGeometryShader.cso", false },
    { L"ms-appx:///BubbleVprtVertexShader.cso", true },
    { L"ms-appx:///TextBillboardVertexShader.cso", true },
    { L"ms-appx:///TextBillboardVprtVertexShader.cso", true },
    { L"ms-appx:///UseDistanceFieldPixelShader.cso", true },
    { L"ms-appx:///TextBillboardGeometryShader.cso", false },
    };
    static bool RequiresViewProjectionBuffer(std::wstring fileName) {
        auto it = requiresViewProjectionBuffer.find(fileName);
        return (*it).second;
    }



    static D3D11_BLEND_DESC                                                        transparentBlendDesc;
    static ID3D11BlendState*                                                       transparentBlendState;
}