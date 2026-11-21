#pragma once

#include "common.h"

#include <cstdint>


namespace render {

class IRenderInterface;


class ITexture : public internal::IResource
{
public:
	struct Desc
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 1;
		uint32_t mipCount = 1;
		ETextureFormat format = ETextureFormat::kUnknown;
	};

public:
	const Desc& getDesc() const { return desc_; }

	uint32_t getWidth() const { return desc_.width; }
	uint32_t getHeight() const { return desc_.height; }

	virtual ~ITexture();

	// Initialize a new single-mip RGBA8 texture from tightly packed pixels.
	virtual void uploadPixels(const void* data, uint32_t size) = 0;

protected:
	ITexture(const Desc& desc);

	const Desc desc_;
};

}  // namespace render
