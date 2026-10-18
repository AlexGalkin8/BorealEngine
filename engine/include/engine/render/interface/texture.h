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

	virtual ~ITexture();

protected:
	ITexture(const Desc& desc);

	const Desc desc_;
};

}  // namespace render
