#pragma once

#include "interface/fence.hpp"

#include <memory>


struct ID3D12Fence;
struct ID3D12CommandQueue;

typedef void* HANDLE;


namespace render {

class D3D12Fence : public IFence
{
public:
    static std::unique_ptr<D3D12Fence> create(IRenderDevice& render_device);

    FenceValue getCompletedValue() const override;

    FenceValue signalOnCpu() override;
    FenceValue signalOnGpu(ID3D12CommandQueue* commandQueue);

    void waitValueOnCpu(FenceValue value) override;
    void waitValueOnGpu(FenceValue value, ID3D12CommandQueue* commandQueue) const;

    ID3D12Fence* const getD3D12Fence() const { return fence_; }

    ~D3D12Fence() override;

private:
    D3D12Fence(ID3D12Fence* fence, HANDLE event_handle);

	void setGpuDebugName() override;

private:
    ID3D12Fence* fence_;
    HANDLE event_handle_;
};

}  // namespace render
