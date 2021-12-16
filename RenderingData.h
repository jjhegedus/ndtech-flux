#pragma once

#include "pch.h"

#include "ShaderStructures.h"
#include "DirectXVertexShader.h"
#include "DirectXPixelShader.h"
#include "DirectXGeometryShader.h"

namespace ndtech {

    struct RenderingData {
        int                                             componentId;
        DirectXVertexShader                             vertexShader;
        DirectXPixelShader                              pixelShader;
        DirectXGeometryShader                           geometryShader;
        winrt::com_ptr<ID3D11Buffer>                    vertexBuffer = nullptr;
        size_t                                          vertexCount = 0;
        UINT                                            vertexStride;
        winrt::com_ptr<ID3D11Buffer>                    indexBuffer = nullptr;
        size_t                                          indexCount = 0;
        DXGI_FORMAT                                     indexBufferFormat;
        D3D_PRIMITIVE_TOPOLOGY                          primitiveTopology;
        winrt::com_ptr<ID3D11InputLayout>               inputLayout = nullptr;
        winrt::com_ptr<ID3D11Buffer>                    modelConstantBuffer = nullptr;
        winrt::com_ptr<ID3D11Buffer>                    frameConstantBuffer = nullptr;
        ID3D11BlendState*                               blendState = nullptr;
        float*                                          blendFactors = nullptr;
        winrt::com_ptr<ID3D11RasterizerState>           rasterizerState = nullptr;
        winrt::com_ptr<ID3D11ShaderResourceView>        pixelShaderResourceView = nullptr;
        winrt::com_ptr<ID3D11SamplerState>              pixelShaderSamplerState = nullptr;
        winrt::Windows::Foundation::Numerics::float3    position;
        bool                                            worldLocked = true;
        bool                                            alwaysRunGeometryShader = false;
        bool                                            drawIndexed = true;
        int                                             zindex = 0;

        bool operator==(const RenderingData& other) const {
            return componentId == other.componentId;
        }

        bool operator<(const RenderingData& other) {
            if (blendState == other.blendState) {
                if (zindex != other.zindex) {
                    return zindex < other.zindex;
                }
                else {
                    return componentId < other.componentId;
                }
            }
            else
            {
                if (blendState == transparentBlendState) {
                    return false;
                }
                else
                {
                    return true;
                }
            }
        }

        bool Equal(const RenderingData& other) const {
            return (
                (componentId == other.componentId) &&
                (vertexShader == other.vertexShader) &&
                (pixelShader == other.pixelShader) &&
                (geometryShader == other.geometryShader) &&
                (vertexBuffer == other.vertexBuffer) &&
                (vertexCount == other.vertexCount) &&
                (vertexStride == other.vertexStride) &&
                (indexBuffer == other.indexBuffer) &&
                (indexCount == other.indexCount) &&
                (indexBufferFormat == other.indexBufferFormat) &&
                (primitiveTopology == other.primitiveTopology) &&
                (inputLayout == other.inputLayout) &&
                (modelConstantBuffer == other.modelConstantBuffer) &&
                (frameConstantBuffer == other.frameConstantBuffer) &&
                (blendState == other.blendState) &&
                (blendFactors == other.blendFactors) &&
                (rasterizerState == other.rasterizerState) &&
                (pixelShaderResourceView == other.pixelShaderResourceView) &&
                (pixelShaderSamplerState == other.pixelShaderSamplerState) &&
                (position == other.position) &&
                (worldLocked == other.worldLocked) &&
                (alwaysRunGeometryShader == other.alwaysRunGeometryShader) &&
                (drawIndexed == other.drawIndexed) &&
                (zindex == other.zindex)
                );
        }

    };

}