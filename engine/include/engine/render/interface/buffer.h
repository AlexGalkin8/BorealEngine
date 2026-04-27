#pragma once

#include "interface/common.h"


namespace render {


class IBuffer : public internal::IResource
{
public:
	struct Desc
	{
		size_t size = 0;
		uint32_t alignment = 0;
	};

public:
	const Desc& getDesc() const { return desc_; }

	virtual void upload(const void* data, size_t size) = 0;

	virtual ~IBuffer();

protected:
	IBuffer(const Desc& desc);

protected:
	const Desc desc_;
};

}  // namespace render
