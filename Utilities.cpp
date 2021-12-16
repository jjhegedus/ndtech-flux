#include "pch.h"

#include "Utilities.h"

namespace ndtech {
    namespace Utilities {
        auto getBenchmarker() {
            return ([](auto lambda, int iterations) {
                unsigned int ui;
                int64_t beginCycles = __rdtscp(&ui);
                for (int i = 0; i < iterations; i++) {
                    lambda();
                }

                int64_t endCycles = __rdtscp(&ui);

                return ((endCycles - beginCycles) / iterations);
            });
        }

        //// Begin: Usage Examples for Benchmarking
        //    // Begin: Setup Benchmarking
        //    int64_t cycles = 0;
        //    int64_t iterations = 100000;
        //    // End: Setup Benchmarking
        //
        //    // Begin: Benchmark Benchmarking
        //    cycles = getBenchmarker ()([]() mutable {
        //    },
        //        iterations);
        //    // End: Benchmark Benchmarking
        //
        //    // Begin: Benchmark Benchmarking
        //    iterations = 100000;
        //    int x;
        //    cycles = getBenchmarker ()([x]() mutable {
        //        x++;
        //    },
        //        iterations);
        //
        //    std::cout << x;
        //    // End: Benchmark Benchmarking
        //
        //    // Begin: Benchmark __rdtsc
        //    int64_t c = 0;
        //    iterations = 1000000;
        //
        //    cycles = getBenchmarker ()([c]() mutable {
        //        c = __rdtsc ();
        //    },
        //        iterations);
        //    std::cout << c;
        //    // End: Benchmark __rdtsc
        //
        //
        //    // Begin: Benchmark QueryPerformanceCounter
        //    iterations = 1000000;
        //
        //    cycles = getBenchmarker ()([]() mutable {
        //        LARGE_INTEGER ticks1;
        //        QueryPerformanceCounter (&ticks1);
        //    },
        //        iterations);
        //    // End: Benchmark QueryPerformanceCounter
        //// End: Usage Examples for Benchmarking

        size_t cache_line_size() {
            size_t line_size = 0;
            DWORD buffer_size = 0;
            DWORD i = 0;
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION * buffer = 0;

            GetLogicalProcessorInformation(0, &buffer_size);
            buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(buffer_size);
            GetLogicalProcessorInformation(&buffer[0], &buffer_size);

            for (i = 0; i != buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
                if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == 1) {
                    line_size = buffer[i].Cache.LineSize;
                    break;
                }
            }

            free(buffer);
            return line_size;
        }

        // Function that reads from a binary file asynchronously using coroutines.
        std::future<std::vector<byte>> ReadDataCoAwait(const std::wstring& filename) {

            TraceLogWrite(
                "Utilities::ReadDataCoAwait",
                TraceLoggingWideString(filename.c_str(), "filename"));

            winrt::Windows::Storage::Streams::IBuffer fileBuffer = co_await winrt::Windows::Storage::PathIO::ReadBufferAsync(winrt::hstring{ filename.c_str() });


            std::vector<byte> returnBuffer;
            returnBuffer.resize(fileBuffer.Length());
            winrt::Windows::Storage::Streams::DataReader::FromBuffer(fileBuffer).ReadBytes(winrt::array_view<byte>(returnBuffer));
            co_return returnBuffer;
        }


        //// Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
        //float ConvertDipsToPixels (float dips, float dpi) {
        //    constexpr float dipsPerInch = 96.0f;
        //    return floorf (dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
        //}

        //Returns the last Win32 error, in string format. Returns an empty string if there is no error.
        std::string GetLastErrorAsString() {
            //Get the error message, if any.
            DWORD errorMessageID = ::GetLastError();
            if (errorMessageID == 0)
                return std::string(); //No error message has been recorded

            LPSTR messageBuffer = nullptr;
            size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

            std::string message(messageBuffer, size);

            //Free the buffer.
            LocalFree(messageBuffer);

            return message;
        }

        std::string GetHRAsString(HRESULT hr) {
            LPSTR messageBuffer = nullptr;
            size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

            std::string message(messageBuffer, size);

            //Free the buffer.
            LocalFree(messageBuffer);

            return message;
        }


        std::string ThreadIdToString(std::thread::id threadId) {
            using namespace std;

            stringstream ss;
            ss << threadId;

            return ss.str();
        }


        DirectX::XMVECTOR XMVECTORFromWinRtFloat3(float3 input) {
            DirectX::XMFLOAT3 xmfloat3 = DirectX::XMFLOAT3(input.x, input.y, input.z);

            return DirectX::XMLoadFloat3(&xmfloat3);
        }

    }
}