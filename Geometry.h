#pragma once

namespace ndtech {
    namespace Geometry {
        void BuildSphereGeometry(std::vector<DirectX::XMFLOAT3> &vertices, std::vector<unsigned short> &indices, float radius, size_t tesselation);
    }
}

