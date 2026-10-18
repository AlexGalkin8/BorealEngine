#pragma once

#include "interface/common.h"
#include "d3d12commandlistpool.hpp"

#include <memory>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>


struct ID3D12CommandQueue;
struct ID3D12CommandList;


namespace render {
class ICommandList;
class IRenderDevice;
class D3D12Fence;
}  // namespace render


namespace render {

template <ECommandListType kType>
class D3D12CommandQueue final : public internal::RenderObject
{
public:
	static std::unique_ptr<D3D12CommandQueue<kType>> create(IRenderDevice& render_device);

	ICommandList& acquireCommandList(const std::string& name);
	FenceValue executePendingCommandLists();
	void addPendingCommandList(ICommandList& command_list);

	void waitValue(FenceValue value);
	void waitGpuIdle();

	D3D12Fence& getD3D12Fence() { return *fence_; }
	ID3D12CommandQueue* getD3D12CommandQueue() const { return command_queue_; }

	~D3D12CommandQueue();

private:
	D3D12CommandQueue(
		ID3D12CommandQueue* command_queue,
		std::unique_ptr<D3D12Fence>&& fence,
		std::unique_ptr<D3D12CommandListPool<kType>>&& command_list_pool_);

	void setGpuDebugName() override;

private:
	ID3D12CommandQueue* command_queue_;
	std::unique_ptr<D3D12Fence> fence_;

	std::unique_ptr<D3D12CommandListPool<kType>> command_list_pool_;

	std::vector<std::reference_wrapper<D3D12CommandList>> pending_command_lists_;
	std::vector<ID3D12CommandList*> d3d12_pending_command_lists_;
};

}  // namespace render
