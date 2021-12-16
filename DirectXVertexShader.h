#pragma once

#include "pch.h"

namespace ndtech {

    static std::atomic<int>                                nextDirectXVertexShaderId = 0;

    struct DirectXVertexShader {
        int id = -1;
        std::wstring fileName;
        winrt::com_ptr<ID3D11VertexShader> shader = nullptr;
        std::vector<::byte> shaderFileData;

        bool operator==(const DirectXVertexShader& other) const {
            return fileName == other.fileName;
        }
    };
}