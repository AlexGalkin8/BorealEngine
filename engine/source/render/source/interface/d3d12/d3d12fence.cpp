#include "d3d12fence.hpp"
#include "d3d12common.hpp"
#include "d3d12renderdevice.hpp"

namespace render {

std::unique_ptr<D3D12Fence> D3D12Fence::create(IRenderDevice& render_device)
{
	D3D12RenderDevice& d3d12_render_interface = static_cast<D3D12RenderDevice&>(render_device);

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	D3D12_ASSERT_RETURN_NULL(d3d12_render_interface.getD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fence->Signal(0);

	HANDLE event_handle = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	if (!event_handle)
	{
		D3D12_ASSERT_RETURN_NULL(HRESULT_FROM_WIN32(GetLastError()));
	}

	return std::unique_ptr<D3D12Fence>(new D3D12Fence(fence.Detach(), event_handle));
}


FenceValue D3D12Fence::getCompletedValue() const
{
	return fence_->GetCompletedValue();
}


FenceValue D3D12Fence::signalOnCpu()
{
	D3D12_ASSERT_RETURN(fence_->Signal(next_signal_value_), kInvalidFenceValue);
	return next_signal_value_++;
}


FenceValue D3D12Fence::signalOnGpu(ID3D12CommandQueue* command_queue)
{
	D3D12_ASSERT_RETURN(command_queue->Signal(fence_, next_signal_value_), kInvalidFenceValue);
	return next_signal_value_++;
}


void D3D12Fence::waitValueOnCpu(FenceValue value)
{
	ASSERT_MSG_RETURN_VOID(value < next_signal_value_, "Wait value has already passed. value=%ull, next_signal_value_=%ull", value, next_signal_value_);
	if (fence_->GetCompletedValue() < value)
	{
		D3D12_ASSERT_RETURN_VOID(fence_->SetEventOnCompletion(value, event_handle_));
		WaitForSingleObject(event_handle_, INFINITE);
	}
}


void D3D12Fence::waitValueOnGpu(uint64_t value, ID3D12CommandQueue* commandQueue) const
{
	ASSERT_MSG_RETURN_VOID(value < next_signal_value_, "Wait value has already passed. value=%ull, next_signal_value_=%ull", value, next_signal_value_);
	D3D12_ASSERT_RETURN_VOID(commandQueue->Wait(fence_, value));
}


D3D12Fence::~D3D12Fence()
{
	if (event_handle_ != nullptr)
	{
		CloseHandle(event_handle_);
		event_handle_ = nullptr;
	}

	D3D12_SAFE_RELEASE(fence_);
}


D3D12Fence::D3D12Fence(ID3D12Fence* fence, HANDLE event_handle)
	: fence_(fence)
	, event_handle_(event_handle)
{
}


void D3D12Fence::setGpuDebugName()
{
	SET_D3D12_RESOURCE_DEBUG_NAME(fence_, getName());
}

}  // namespace render
