#pragma once

#include "interface/common.h"


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
	virtual ~ITexture();

	const Desc& getDesc() const { return desc_; }
	ITexture(const Desc& desc);

protected:
	const Desc desc_;
};

}  // namespace render
