#pragma once

#include "interface/common.h"

#include <memory>


namespace render {
class IRenderDevice;
}  // namespace render


namespace render {

static constexpr FenceValue kInvalidFenceValue = std::numeric_limits<FenceValue>::max();


class IFence : public internal::RenderObject
{
public:
	template<ERenderAPI render_api>
	static std::unique_ptr<IFence> create(IRenderDevice& render_device);

	FenceValue getNextSignalValue() const { return next_signal_value_; }

	virtual FenceValue getCompletedValue() const = 0;

	virtual FenceValue signalOnCpu() = 0;

	virtual void waitValueOnCpu(FenceValue value) = 0;

	virtual ~IFence();

protected:
	IFence();

protected:
	FenceValue next_signal_value_;
};

}  // namespace render
