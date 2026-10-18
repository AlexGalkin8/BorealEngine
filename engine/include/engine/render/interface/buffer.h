#pragma once

#include "common.h"

#include <cstdint>


namespace render {


enum class EBufferUsage : uint32_t
{
	kVertex = 1 << 0,
	kIndex = 1 << 1,
	kConstant = 1 << 2,
	kStructured = 1 << 3,
	kRaw = 1 << 4,
	kUnorderedAccess = 1 << 5,
};
ENUM_CLASS_BITWISE_OPERATORS(EBufferUsage);


class IBuffer : public internal::IResource
{
public:
	struct Desc
	{
		size_t       size = 0;
		uint32_t     alignment = 0;
		uint32_t     stride = 0;
		EBufferUsage usage = EBufferUsage::kVertex;
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
