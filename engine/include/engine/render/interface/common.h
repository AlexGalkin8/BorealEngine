#pragma once

#include <engine/core/utility/assert.h>
#include <engine/core/utility/utility.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <atomic>
#include <limits>
#include <format>
#include <utility>


namespace render {

enum class ERenderAPI
{
	kD3D12 = 0,
	//kVulkan,
};

enum class ETextureFormat
{
	kUnknown = 0,

	kRG8_Unorm,
	kRGBA8_Unorm,
	kRGBA8_Unorm_Srgb,
	kRGBA8_Snorm,

	kRG16_Float,
	kRGBA16_Float,
	kRGBA16_SNorm,

	kRG32_Float,
	kRGB32_Float,
	kRGBA32_Float,

	D16_Unorm,
	D24_Unorm_S8_Uint,
	D32_Float,
};

enum class EShaderType
{
	kVertex = 0,
	kPixel,
	kDomain,
	kHull,
	kGeometry,

	kCompute,

	kAmplification,
	kMesh
};

enum class EPrimitiveTopology
{
	kPoint = 0,
	kLine,
	kTriangle,
	kPatch
};


enum class ECommandListType
{
	kDirect = 0,
	kCompute,
	kCopy
};


using FenceValue = uint64_t;

inline constexpr uint32_t kMaxRenderTargets = 8;

inline constexpr uint32_t kBackBufferCount = 3;
inline constexpr ETextureFormat kBackBufferFormat = ETextureFormat::kRGBA8_Unorm;


inline bool isDepthTextureFormat(ETextureFormat format)
{
	switch (format)
	{
		case ETextureFormat::D16_Unorm:
		case ETextureFormat::D24_Unorm_S8_Uint:
		case ETextureFormat::D32_Float:
			return true;
		default:
			return false;
	}
}


namespace internal {

using ResourceID = uint64_t;
inline constexpr ResourceID kInvalidResourceID = (std::numeric_limits<ResourceID>::max)();


class RenderObject : public utility::NonCopyable
{
public:
	const std::string& getName() const { return name_; }
	ResourceID getID() const { return id_; }

	void setName(const std::string& name)
	{
		name_ = name;
#ifdef _DEBUG
		setGpuDebugName();
#endif
	}

protected:
	RenderObject()
		: id_(++next_id_)
		, name_("UnnamedRenderObject[id=" + std::to_string(id_) + "]") {}

	virtual ~RenderObject() = default;

	// For debug purposes, sets the name of the resource in the GPU API (e.g., D3D12) for easier identification in graphics debuggers.
	virtual void setGpuDebugName() = 0;

private:
	std::string name_;
	ResourceID id_;
	inline static std::atomic<ResourceID> next_id_{ 0 };
};


class IResource : public RenderObject
{
protected:
	IResource() = default;
	virtual ~IResource() = default;
};

#ifdef _DEBUG

template<typename TObject>
inline void setRenderObjectDebugNameImpl(TObject& obj, std::string_view name)
{
	obj.setName(std::string(name));
}


template<typename TObject, typename... TArgs>
inline void setRenderObjectDebugNameImpl(TObject& obj, std::format_string<TArgs...> fmt, TArgs&&... args)
{
	obj.setName(std::format(fmt, std::forward<TArgs>(args)...));
}

#endif // _DEBUG

}  // namespace internal

#ifdef _DEBUG

#define SET_RENDER_OBJECT_DEBUG_NAME(obj, ...) \
	do { ::render::internal::setRenderObjectDebugNameImpl((obj), __VA_ARGS__); } while (0)

#else

#define SET_RENDER_OBJECT_DEBUG_NAME(obj, ...) \
	do { (void)(obj); } while (0)

#endif // _DEBUG



}  // namespace render
