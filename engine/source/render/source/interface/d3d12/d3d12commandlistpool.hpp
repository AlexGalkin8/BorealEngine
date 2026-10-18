#pragma once

#include "interface/common.h"

#include "d3d12fence.hpp"

#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <span>
#include <functional>


namespace render {
class IRenderDevice;
class D3D12CommandList;
}  // namespace render


namespace render {

template <ECommandListType kType>
class D3D12CommandListPool final : public utility::NonCopyable
{
public:
	static std::unique_ptr<D3D12CommandListPool> create(IRenderDevice& render_device, D3D12Fence& fence)
	{
		return std::unique_ptr<D3D12CommandListPool>(new D3D12CommandListPool(render_device, fence));
	}

	D3D12CommandList& acquire(const std::string& name);
	void release(std::span<const std::reference_wrapper<D3D12CommandList>> command_lists, FenceValue fence_value);

	~D3D12CommandListPool() = default;

private:
	explicit D3D12CommandListPool(IRenderDevice& render_device, D3D12Fence& fence);

private:
	IRenderDevice& render_device_;
	D3D12Fence& fence_;

	std::vector<std::unique_ptr<D3D12CommandList>> command_lists_;
	std::queue<std::reference_wrapper<D3D12CommandList>> executing_command_lists_;
};



}  // namespace render
