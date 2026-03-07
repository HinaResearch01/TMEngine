#pragma once

// Windows
#include <Windows.h>

// STL
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>
#include <format>
#include <iostream>
#include <cassert>
#include <codecvt>
#include <fstream>

// DX
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// Json
#include <Json/json.hpp>

// Logger
#include "Engine/Core/Logger/LogMacros.h"

// StringUtil
#include "Engine/Core/String/StringUtil.h"