#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

// windows headers
#include <wrl.h>

// std headers
#include <cstdarg>
#include <cstdio>
#include <string>

#include "interface/common.h"


namespace render {

inline constexpr uint32_t kD3D12RTVDescriptorCount = 1024;
inline constexpr uint32_t kD3D12SRVDescriptorCount = 1024;
inline constexpr uint32_t kCommandListReservedDescriptorCount = 64;


#define D3D12_SAFE_RELEASE(obj) if (obj) { (obj)->Release(); (obj) = nullptr; }


inline constexpr D3D12_COMMAND_LIST_TYPE translateCommandListType(ECommandListType type)
{
	switch (type)
	{
		case ECommandListType::kDirect:  return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case ECommandListType::kCompute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case ECommandListType::kCopy:    return D3D12_COMMAND_LIST_TYPE_COPY;
		default:                         return D3D12_COMMAND_LIST_TYPE_DIRECT;
	}
}

inline constexpr const char* translateCommandListTypeToString(ECommandListType type)
{
	switch (type)
	{
		case ECommandListType::kDirect:  return "Direct";
		case ECommandListType::kCompute: return "Compute";
		case ECommandListType::kCopy:    return "Copy";
		default:                         return "Direct";
	}
}


namespace internal {

class D3D12Resource
{
protected:

};


inline constexpr DXGI_FORMAT translateTextureFormatToDxgiFormat(ETextureFormat format)
{
	switch (format)
	{
		// Unknown / invalid
		case ETextureFormat::kUnknown:
			return DXGI_FORMAT_UNKNOWN;

		// 8-bit single channel
		//case ETextureFormat::kR8_Unorm:             return DXGI_FORMAT_R8_UNORM;
		//case ETextureFormat::kR8_Snorm:             return DXGI_FORMAT_R8_SNORM;
		//case ETextureFormat::R8_Uint:              return DXGI_FORMAT_R8_UINT;
		//case ETextureFormat::R8_Sint:              return DXGI_FORMAT_R8_SINT;

		// 8-bit two channels
		case ETextureFormat::kRG8_Unorm:          return DXGI_FORMAT_R8G8_UNORM;
		//case ETextureFormat::R8G8_SNORM:          return DXGI_FORMAT_R8G8_SNORM;
		//case ETextureFormat::R8G8_UINT:           return DXGI_FORMAT_R8G8_UINT;
		//case ETextureFormat::R8G8_SINT:           return DXGI_FORMAT_R8G8_SINT;

		// 8-bit four channels
		case ETextureFormat::kRGBA8_Unorm:      return DXGI_FORMAT_R8G8B8A8_UNORM;
		case ETextureFormat::kRGBA8_Unorm_Srgb: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case ETextureFormat::kRGBA8_Snorm:      return DXGI_FORMAT_R8G8B8A8_SNORM;
		//case ETextureFormat::R8G8B8A8_UINT:       return DXGI_FORMAT_R8G8B8A8_UINT;
		//case ETextureFormat::R8G8B8A8_SINT:       return DXGI_FORMAT_R8G8B8A8_SINT;

		// BGR(A) variants
		//case ETextureFormat::B8G8R8A8_UNORM:      return DXGI_FORMAT_B8G8R8A8_UNORM;
		//case ETextureFormat::B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		//case ETextureFormat::B8G8R8X8_UNORM:      return DXGI_FORMAT_B8G8R8X8_UNORM;

		// 16-bit single channel
		//case ETextureFormat::R16_FLOAT:           return DXGI_FORMAT_R16_FLOAT;
		//case ETextureFormat::R16_UNORM:           return DXGI_FORMAT_R16_UNORM;
		//case ETextureFormat::R16_SNORM:           return DXGI_FORMAT_R16_SNORM;
		//case ETextureFormat::R16_UINT:            return DXGI_FORMAT_R16_UINT;
		//case ETextureFormat::R16_SINT:            return DXGI_FORMAT_R16_SINT;

		// 16-bit two channels
		case ETextureFormat::kRG16_Float:        return DXGI_FORMAT_R16G16_FLOAT;
		//case ETextureFormat::R16G16_UNORM:        return DXGI_FORMAT_R16G16_UNORM;
		//case ETextureFormat::R16G16_SNORM:        return DXGI_FORMAT_R16G16_SNORM;
		//case ETextureFormat::R16G16_UINT:         return DXGI_FORMAT_R16G16_UINT;
		//case ETextureFormat::R16G16_SINT:         return DXGI_FORMAT_R16G16_SINT;

		// 16-bit four channels
		case ETextureFormat::kRGBA16_Float:  return DXGI_FORMAT_R16G16B16A16_FLOAT;
		//case ETextureFormat::R16G16B16A16_UNORM:  return DXGI_FORMAT_R16G16B16A16_UNORM;
		case ETextureFormat::kRGBA16_SNorm:  return DXGI_FORMAT_R16G16B16A16_SNORM;
		//case ETextureFormat::R16G16B16A16_UINT:   return DXGI_FORMAT_R16G16B16A16_UINT;
		//case ETextureFormat::R16G16B16A16_SINT:   return DXGI_FORMAT_R16G16B16A16_SINT;

		// 32-bit single channel
		//case ETextureFormat::R32_FLOAT:           return DXGI_FORMAT_R32_FLOAT;
		//case ETextureFormat::R32_UINT:            return DXGI_FORMAT_R32_UINT;
		//case ETextureFormat::R32_SINT:            return DXGI_FORMAT_R32_SINT;

		// 32-bit two channels
		case ETextureFormat::kRG32_Float:        return DXGI_FORMAT_R32G32_FLOAT;
		//case ETextureFormat::R32G32_UINT:         return DXGI_FORMAT_R32G32_UINT;
		//case ETextureFormat::R32G32_SINT:         return DXGI_FORMAT_R32G32_SINT;

		// 32-bit three channels
		case ETextureFormat::kRGB32_Float:       return DXGI_FORMAT_R32G32B32_FLOAT;

		// 32-bit four channels
		case ETextureFormat::kRGBA32_Float:  return DXGI_FORMAT_R32G32B32A32_FLOAT;
		//case ETextureFormat::R32G32B32A32_UINT:   return DXGI_FORMAT_R32G32B32A32_UINT;
		//case ETextureFormat::R32G32B32A32_SINT:   return DXGI_FORMAT_R32G32B32A32_SINT;

		// Depth-stencil
		case ETextureFormat::D16_Unorm:           return DXGI_FORMAT_D16_UNORM;
		case ETextureFormat::D24_Unorm_S8_Uint:   return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case ETextureFormat::D32_Float:           return DXGI_FORMAT_D32_FLOAT;
		//case ETextureFormat::D32_FLOAT_S8X24_UINT:return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		// Block compressed formats (BC)
		//case ETextureFormat::BC1_UNORM:           return DXGI_FORMAT_BC1_UNORM;
		//case ETextureFormat::BC1_UNORM_SRGB:      return DXGI_FORMAT_BC1_UNORM_SRGB;
		//case ETextureFormat::BC2_UNORM:           return DXGI_FORMAT_BC2_UNORM;
		//case ETextureFormat::BC2_UNORM_SRGB:      return DXGI_FORMAT_BC2_UNORM_SRGB;
		//case ETextureFormat::BC3_UNORM:           return DXGI_FORMAT_BC3_UNORM;
		//case ETextureFormat::BC3_UNORM_SRGB:      return DXGI_FORMAT_BC3_UNORM_SRGB;
		//case ETextureFormat::BC4_UNORM:           return DXGI_FORMAT_BC4_UNORM;
		//case ETextureFormat::BC4_SNORM:           return DXGI_FORMAT_BC4_SNORM;
		//case ETextureFormat::BC5_UNORM:           return DXGI_FORMAT_BC5_UNORM;
		//case ETextureFormat::BC5_SNORM:           return DXGI_FORMAT_BC5_SNORM;
		//case ETextureFormat::BC6H_UF16:           return DXGI_FORMAT_BC6H_UF16;
		//case ETextureFormat::BC6H_SF16:           return DXGI_FORMAT_BC6H_SF16;
		//case ETextureFormat::BC7_UNORM:           return DXGI_FORMAT_BC7_UNORM;
		//case ETextureFormat::BC7_UNORM_SRGB:      return DXGI_FORMAT_BC7_UNORM_SRGB;

		// Fallback for any future/unknown formats
		default:
			return DXGI_FORMAT_UNKNOWN;
	}
}

}  // namespace internal

#if _DEBUG

#include <dxgidebug.h>

namespace {
inline void printLogImpl(const char* fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
	va_end(ap);
	OutputDebugStringA(buf);
}

inline void checkD3D12CallImpl(HRESULT expr_ret_value, const char* expr, const char* file_name, const char* caller_func_name, unsigned int line)
{
	printLogImpl("D3D12 failed: %s HRESULT=0x%08X\nAt: %s:%u (%s)\n",
		expr,
		static_cast<unsigned int>(expr_ret_value),
		file_name,
		line,
		caller_func_name);
	DebugBreak();
}

template<class DxObject>
inline void setD3D12ResourceDebugNameImpl(DxObject* resource, const char* fmt, ...)
{
	if (resource && fmt)
	{
		char buf[256];
		va_list ap;
		va_start(ap, fmt);
		vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
		va_end(ap);

		std::wstring toWCharStr(buf, buf + strlen(buf));
		resource->SetName(toWCharStr.c_str());
	}
}

template<class DxObject>
inline void setD3D12ResourceDebugNameImpl(DxObject* resource, std::string_view name)
{
	if (!name.empty() && resource)
	{
		std::wstring toWCharStr(name.begin(), name.end());
		resource->SetName(toWCharStr.c_str());
	}
}

}

#define D3D12_ASSERT(expr) \
	do { if (FAILED(expr)) { checkD3D12CallImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__); } } while (0)

#define D3D12_ASSERT_RETURN(expr, retValue) \
	do { if (FAILED(expr)) { checkD3D12CallImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__); return (retValue); } } while (0)

#define D3D12_ASSERT_RETURN_VOID(expr) \
	do { if (FAILED(expr)) { checkD3D12CallImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__); return; } } while (0)

#define D3D12_ASSERT_RETURN_NULL(expr) \
	do { if (FAILED(expr)) { checkD3D12CallImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__); return nullptr; } } while (0)

#define PRINT_LOG(fmt, ...) printLogImpl((fmt), ##__VA_ARGS__)

#define SET_D3D12_RESOURCE_DEBUG_NAME(obj, fmt, ...) setD3D12ResourceDebugNameImpl((obj), (fmt), ##__VA_ARGS__);

#else

namespace {
template<typename... Args>
inline void ignoreRelease(Args&&...) noexcept {}
}

#define D3D12_ASSERT(expr) (void)ignoreRelease((expr))
#define D3D12_ASSERT_RETURN(expr, retValue)  do { if (!(cond)) { return (retValue); } } while (0)

#define PRINT_LOG(fmt, ...) (void)ignoreRelease((fmt), ##__VA_ARGS__)

#define SET_D3D12_RESOURCE_DEBUG_NAME(obj, name) (void)ignoreRelease((obj), (name))

#endif // _DEBUG

}  // namespace render
