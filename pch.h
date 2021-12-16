#pragma once

#include "targetver.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// UWP Required
#include <windows.h>

// Trace Logging
#include <TraceLoggingProvider.h>

TRACELOGGING_DECLARE_PROVIDER(traceProvider);

#define TraceLogWrite(eventName, ...) \
    _TlgWrite_imp(_TlgWrite, \
    traceProvider, eventName, \
    (NULL, NULL), \
    __VA_ARGS__)

// Basic Includes
#include <atomic>
#include <string>
#include <any>
#include <thread>
#include <iostream>
#include <sstream>
#include <future>

// winrt includes
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Numerics.h"

// DirectX Includes
#include <DirectXMath.h>
#include <dxgi.h>
#include <d3d11.h>
#include <DirectXCollision.h>