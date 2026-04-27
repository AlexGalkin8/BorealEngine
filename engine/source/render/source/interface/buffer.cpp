#include "interface/buffer.h"


namespace render {

IBuffer::IBuffer(const Desc& desc)
	: desc_(desc)
{
}


IBuffer::~IBuffer()
{
}

}  // namespace render
