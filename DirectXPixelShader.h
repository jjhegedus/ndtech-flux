#pragma once

#include "pch.h"

namespace ndtech {

    static std::atomic<int>                                nextDirectXPixelShaderId = 0;

    struct DirectXPixelShader {
        int id = -1;
        std::wstring fileName;
        winrt::com_ptr<ID3D11PixelShader> shader = nullptr;
        std::vector<::byte> shaderFileData;

        bool operator==(const DirectXPixelShader& other) const {
            return fileName == other.fileName;
        }
    };

}