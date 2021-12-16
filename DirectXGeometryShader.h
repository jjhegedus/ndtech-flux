#pragma once

#include "pch.h"

namespace ndtech {

    static std::atomic<int>                                nextDirectXGeometryShaderId = 0;

    struct DirectXGeometryShader {

        int id = -1;
        std::wstring fileName;
        winrt::com_ptr<ID3D11GeometryShader> shader = nullptr;
        std::vector<::byte> shaderFileData;

        bool operator==(const DirectXGeometryShader& other) const {
            return fileName == other.fileName;
        }
    };

}