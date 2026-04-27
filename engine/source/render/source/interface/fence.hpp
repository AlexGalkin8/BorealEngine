#pragma once

#include "interface/common.h"

#include <memory>


namespace render {
class IRenderDevice;
}  // namespace render


namespace render {
static constexpr uint64_t kInvalidFenceValue = std::numeric_limits<uint64_t>::max();


class IFence : public internal::RenderObject
{
public:
	template<ERenderAPI render_api>
	static std::unique_ptr<IFence> create(IRenderDevice& render_device);

	uint64_t getNextSignalValue() const { return next_signal_value_; }

	virtual uint64_t getCompletedValue() const = 0;

	virtual uint64_t signalOnCpu() = 0;

	virtual void waitValueOnCpu(uint64_t value) = 0;

	virtual ~IFence();

protected:
	IFence();

protected:
	uint64_t next_signal_value_;
};

}  // namespace render
