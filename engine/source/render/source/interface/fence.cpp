#include "fence.hpp"
#include "d3d12/d3d12fence.hpp"


namespace render {

template<ERenderAPI>
std::unique_ptr<IFence> IFence::create(IRenderDevice& render_device) = delete;


template<>
std::unique_ptr<IFence> IFence::create<ERenderAPI::kD3D12>(IRenderDevice& render_device)
{
	return D3D12Fence::create(render_device);
}


IFence::~IFence()
{
}


IFence::IFence()
	: next_signal_value_(1)
{
}

}  // namespace render