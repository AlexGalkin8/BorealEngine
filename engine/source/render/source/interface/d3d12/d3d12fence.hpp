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

    uint64_t getCompletedValue() const override;

    uint64_t signalOnCpu() override;
    uint64_t signalOnGpu(ID3D12CommandQueue* commandQueue);

    void waitValueOnCpu(uint64_t value) override;
    void waitValueOnGpu(uint64_t value, ID3D12CommandQueue* commandQueue) const;

    ID3D12Fence* const getFence() const { return fence_; }

    ~D3D12Fence() override;

private:
    D3D12Fence(ID3D12Fence* fence, HANDLE event_handle);

	void setGpuDebugName() override;

private:
    ID3D12Fence* fence_;
    HANDLE event_handle_;
};

}  // namespace render
