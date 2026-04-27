#pragma once

#include <engine/core/utility/assert.h>
#include <engine/core/utility/utility.h>

#include <cstdint>
#include <string>
#include <atomic>
#include <limits>


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

inline constexpr uint32_t kMaxRenderTargets = 8;

inline constexpr uint32_t kBackBufferCount = 3;
inline constexpr ETextureFormat kBackBufferFormat = ETextureFormat::kRGBA8_Unorm;


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

}  // namespace internal

}  // namespace render
