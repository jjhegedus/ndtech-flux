#include "pch.h"
#include "Geometry.h"


namespace ndtech {
    namespace Geometry {

        using namespace DirectX;

        inline void CheckIndexOverflow(size_t value)
        {
            // Use >=, not > comparison, because some D3D level 9_x hardware does not support 0xFFFF index values.
            if (value >= USHRT_MAX)
                throw std::exception("Index value out of range: cannot tesselate primitive so finely");
        }

        // Collection types used when generating the geometry.
        inline void index_push_back(std::vector<unsigned short>& indices, size_t value)
        {
            CheckIndexOverflow(value);
            indices.push_back(static_cast<uint16_t>(value));
        }

        void BuildSphereGeometry(std::vector<DirectX::XMFLOAT3> &vertices, std::vector<unsigned short> &indices, float radius, size_t tessellation) {
            vertices.clear();
            indices.clear();

            if (tessellation < 3)
                throw std::out_of_range("tesselation parameter out of range");

            size_t verticalSegments = tessellation;
            size_t horizontalSegments = tessellation * 2;

            // Create rings of vertices at progressively higher latitudes.
            for (size_t i = 0; i <= verticalSegments; i++)
            {
                //float v = 1 - float(i) / verticalSegments;

                float latitude = (i * XM_PI / verticalSegments) - XM_PIDIV2;
                float dy, dxz;

                XMScalarSinCos(&dy, &dxz, latitude);

                // Create a single ring of vertices at this latitude.
                for (size_t j = 0; j <= horizontalSegments; j++)
                {
                    float longitude = j * XM_2PI / horizontalSegments;
                    float dx, dz;

                    XMScalarSinCos(&dx, &dz, longitude);

                    dx *= dxz;
                    dz *= dxz;

                    XMVECTOR normal = DirectX::XMVectorSet(dx, dy, dz, 0);
                    XMVECTOR scaledNormal = XMVectorScale(normal, radius);

                    XMFLOAT3 position;
                    XMStoreFloat3(&position, scaledNormal);

                    vertices.push_back(position);
                }
            }

            // Fill the index buffer with triangles joining each pair of latitude rings.
            size_t stride = horizontalSegments + 1;

            for (size_t i = 0; i < verticalSegments; i++)
            {
                for (size_t j = 0; j <= horizontalSegments; j++)
                {
                    size_t nextI = i + 1;
                    size_t nextJ = (j + 1) % stride;

                    index_push_back(indices, i * stride + j);
                    index_push_back(indices, nextI * stride + j);
                    index_push_back(indices, i * stride + nextJ);

                    index_push_back(indices, i * stride + nextJ);
                    index_push_back(indices, nextI * stride + j);
                    index_push_back(indices, nextI * stride + nextJ);
                }
            }
        }
    }
}
