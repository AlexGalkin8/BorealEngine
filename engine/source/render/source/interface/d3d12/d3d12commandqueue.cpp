#include "d3d12commandqueue.hpp"
#include "d3d12common.hpp"
#include "d3d12commandlist.hpp"
#include "d3d12fence.hpp"
#include "d3d12renderdevice.hpp"
#include "interface/fence.hpp"

#include <engine/core/utility/enum.h>

#include <algorithm>
#include <iterator>


namespace render {

static constexpr utility::EnumMapping<ECommandListType, D3D12_COMMAND_LIST_TYPE > kD3D12CommandListTypeMapping[] = {
	{ ECommandListType::kDirect,  D3D12_COMMAND_LIST_TYPE_DIRECT },
	{ ECommandListType::kCompute, D3D12_COMMAND_LIST_TYPE_COMPUTE },
	{ ECommandListType::kCopy,    D3D12_COMMAND_LIST_TYPE_COPY }
};


static constexpr utility::EnumMapping<ECommandListType, const char*> kCommandListTypeNamesMapping[] = {
	{ ECommandListType::kDirect,  "Direct" },
	{ ECommandListType::kCompute, "Compute" },
	{ ECommandListType::kCopy,    "Copy" }
};


template<ECommandListType kType>
std::unique_ptr<D3D12CommandQueue<kType>> D3D12CommandQueue<kType>::create(IRenderDevice& render_device)
{
	ID3D12Device9* device = static_cast<D3D12RenderDevice&>(render_device).getD3D12Device();

	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.Type = kD3D12CommandListTypeMapping[static_cast<size_t>(kType)].value;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
	D3D12_ASSERT_RETURN_NULL(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));

	std::unique_ptr<D3D12Fence> fence = D3D12Fence::create(render_device);
	ASSERT_MSG_RETURN_NULL(fence, "Failed to create fence for D3D12CommandQueue");

	std::unique_ptr<D3D12CommandListPool<kType>> command_list_pool =
		D3D12CommandListPool<kType>::create(render_device, *fence);
	ASSERT_MSG_RETURN_NULL(command_list_pool, "D3D12RenderDevice: failed to create command list pool");

	return std::unique_ptr<D3D12CommandQueue<kType>>(new D3D12CommandQueue<kType>(command_queue.Detach(), std::move(fence), std::move(command_list_pool)));
}


template<ECommandListType kType>
ICommandList& D3D12CommandQueue<kType>::acquireCommandList(const std::string& name)
{
	return command_list_pool_->acquire(name);
}


template<ECommandListType kType>
FenceValue D3D12CommandQueue<kType>::executePendingCommandLists()
{
	if (d3d12_pending_command_lists_.empty())
	{
		return 0;
	}

	command_queue_->ExecuteCommandLists(static_cast<UINT>(d3d12_pending_command_lists_.size()), d3d12_pending_command_lists_.data());
	
	FenceValue signal_value = fence_->signalOnGpu(command_queue_);
	command_list_pool_->release(pending_command_lists_, signal_value);

	pending_command_lists_.clear();
	d3d12_pending_command_lists_.clear();
	return signal_value;
}


template<ECommandListType kType>
void D3D12CommandQueue<kType>::addPendingCommandList(ICommandList& command_list)
{
	D3D12CommandList& d3d12_command_list = static_cast<D3D12CommandList&>(command_list);
	pending_command_lists_.push_back(d3d12_command_list);
	d3d12_pending_command_lists_.push_back(d3d12_command_list.getD3D12CommandList());
}


template<ECommandListType kType>
void D3D12CommandQueue<kType>::waitValue(FenceValue value)
{
	fence_->waitValueOnCpu(value);
}


template<ECommandListType kType>
void D3D12CommandQueue<kType>::waitGpuIdle()
{
	FenceValue signal_value = static_cast<D3D12Fence*>(fence_.get())->signalOnGpu(command_queue_);
	fence_->waitValueOnCpu(signal_value);
}


template<ECommandListType kType>
D3D12CommandQueue<kType>::~D3D12CommandQueue()
{
	waitGpuIdle();
	D3D12_SAFE_RELEASE(command_queue_);
}


template<ECommandListType kType>
D3D12CommandQueue<kType>::D3D12CommandQueue(
	ID3D12CommandQueue* command_queue,
	std::unique_ptr<D3D12Fence>&& fence,
	std::unique_ptr<D3D12CommandListPool<kType>>&& command_list_pool_)
	: command_queue_(command_queue)
	, fence_(std::move(fence))
	, command_list_pool_(std::move(command_list_pool_))
{
}


template<ECommandListType kType>
void D3D12CommandQueue<kType>::setGpuDebugName()
{
	SET_D3D12_RESOURCE_DEBUG_NAME(command_queue_, "D3D12CommandQueue<%s>::command_queue_", kCommandListTypeNamesMapping[static_cast<size_t>(kType)].value);
}


template class D3D12CommandQueue<ECommandListType::kDirect>;
template class D3D12CommandQueue<ECommandListType::kCompute>;
template class D3D12CommandQueue<ECommandListType::kCopy>;

}  // namespace render
