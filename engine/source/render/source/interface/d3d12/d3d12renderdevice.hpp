#pragma once

#include "interface/renderdevice.h"


struct ID3D12Device9;
struct IDXGIFactory6;
struct IDXGIAdapter4;
struct ID3D12DescriptorHeap;


namespace render {

class D3D12RenderDevice final : public IRenderDevice
{
    friend class IRenderDevice;
    friend class D3D12SwapChain;
    friend class D3D12DeviceContext;
    friend class D3D12ResourceManager;
    friend class D3D12PipelineStateManager;

public:
    std::unique_ptr<ISwapChain> createSwapChain(const platform::IWindow& window) override;

    ID3D12Device9* getDevice() const { return device_; }
    IDXGIFactory6* getDxgiFactory() const { return dxgi_factory_; }

    ID3D12DescriptorHeap* getRtvHeap() const { return rtv_heap_; }
    ID3D12DescriptorHeap* getDsvHeap() const { return dsv_heap_; }
    ID3D12DescriptorHeap* getSrvHeap() const { return srv_heap_; }

    ~D3D12RenderDevice() override;

private:
    D3D12RenderDevice();

    static std::unique_ptr<D3D12RenderDevice> create(const CreationDesc& creation_desc);

private:
    IDXGIFactory6* dxgi_factory_;
    IDXGIAdapter4* adapter_;
    ID3D12Device9* device_;

    ID3D12DescriptorHeap* rtv_heap_;
    ID3D12DescriptorHeap* dsv_heap_;
    ID3D12DescriptorHeap* srv_heap_;
};

}  // namespace render
