#include "interface/texture.h"


namespace render {

ITexture::ITexture(const Desc& desc)
	: desc_(desc)
{
}


ITexture::~ITexture()
{
}

}  // namespace render
