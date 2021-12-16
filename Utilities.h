#pragma once

#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.Streams.h"

//#include <inspectable.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Numerics;

namespace ndtech {
    namespace Utilities {
        auto getBenchmarker();
        size_t cache_line_size();
        std::future<std::vector<byte>> ReadDataCoAwait(const std::wstring& filename);
        //float ConvertDipsToPixels(float dips, float dpi);
        std::string GetLastErrorAsString();
        std::string GetHRAsString(HRESULT hr);
        std::string ThreadIdToString(std::thread::id threadId);
        static bool inline NotEqual(const winrt::Windows::Foundation::Size lhs, const winrt::Windows::Foundation::Size rhs)
        {
            if (lhs.Height != rhs.Height)
                return false;

            if (lhs.Width != rhs.Width)
                return false;

            return true;
        }
        DirectX::XMVECTOR XMVECTORFromWinRtFloat3(float3 input);

        template <typename ExternalType>
        size_t GetVectorElementSize(std::vector<ExternalType> vector) {
            return sizeof(ExternalType);
        }
    }
}
