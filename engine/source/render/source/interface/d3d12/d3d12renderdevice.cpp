#include "d3d12renderdevice.hpp"
#include "d3d12common.hpp"
#include "d3d12swapchain.hpp"
#include "d3d12fence.hpp"

#include <engine/platform/window.h>


namespace render {

#if _DEBUG
static void initDebugLayer(bool enable_validation)
{
    Microsoft::WRL::ComPtr<ID3D12Debug> debug_layer;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_layer))))
    {
        debug_layer->EnableDebugLayer();
        PRINT_LOG("D3D12 debug validation layer enabled\n");

        if (enable_validation)
        {
            Microsoft::WRL::ComPtr<ID3D12Debug1> debug_layer_1;
            if (SUCCEEDED((debug_layer->QueryInterface(IID_PPV_ARGS(&debug_layer_1)))))
            {
                debug_layer_1->SetEnableGPUBasedValidation(true);
                PRINT_LOG("D3D12 GPU-based validation enabled\n");
            }
        }
    }
    else
    {
        PRINT_LOG("WARNING: Unable to enable D3D12 debug validation layer\n");
    }
}


static void initDxgiDebug(DWORD& out_dxgi_debug_flags)
{
    Microsoft::WRL::ComPtr<IDXGIInfoQueue> dxgi_info_queue;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgi_info_queue.GetAddressOf()))))
    {
        out_dxgi_debug_flags = DXGI_CREATE_FACTORY_DEBUG;

        dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

        DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
        {
            80 /* IDXGISwapChain::GetContainingOutput */,
        };
        DXGI_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        dxgi_info_queue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
    }
}


static bool isWindowsDeveloperModeEnabled()
{
    HKEY h_key;
    LSTATUS result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_READ, &h_key);
    if (result == ERROR_SUCCESS)
    {
        DWORD keyValue, keySize = sizeof(DWORD);
        result = RegQueryValueEx(h_key, L"AllowDevelopmentWithoutDevLicense", 0, NULL, (byte*)&keyValue, &keySize);
        if (result == ERROR_SUCCESS && keyValue == 1)
        {
            RegCloseKey(h_key);
            return true;
        }
        RegCloseKey(h_key);
    }
    return false;
}
#endif // _DEBUG


static HRESULT initBestGpuAdapter(IDXGIFactory6* dxgi_factory, Microsoft::WRL::ComPtr<IDXGIAdapter4>& out_adapter)
{
    HRESULT hr = DXGI_ERROR_NOT_FOUND;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter;
    for (UINT adapter_index = 0;
        dxgi_factory->EnumAdapterByGpuPreference(adapter_index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
        ++adapter_index)
    {
        DXGI_ADAPTER_DESC3 desc;
        adapter->GetDesc3(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;
        out_adapter = adapter;
        hr = S_OK;
        break;
    }
    return hr;
}


static void printAdapterInfo(IDXGIAdapter4* adapter)
{
    DXGI_ADAPTER_DESC3 desc;
    adapter->GetDesc3(&desc);
    PRINT_LOG("Selected GPU: %ls\n", desc.Description);
}


static bool isDirectXRaytracingSupport(ID3D12Device* device)
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 feature_support = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_support, sizeof(feature_support))))
        return false;
    return feature_support.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}


std::unique_ptr<D3D12RenderDevice> D3D12RenderDevice::create(const CreationDesc& creation_desc)
{
    DWORD dxgi_factory_flags = 0;

#if _DEBUG
    initDebugLayer(creation_desc.enable_debug_validation_layer);
    initDxgiDebug(dxgi_factory_flags);
#endif

    Microsoft::WRL::ComPtr<IDXGIFactory6> dxgi_factory;
    D3D12_ASSERT_RETURN_NULL(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));

    Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter;
    D3D12_ASSERT_RETURN_NULL(initBestGpuAdapter(dxgi_factory.Get(), adapter));

    printAdapterInfo(adapter.Get());

    Microsoft::WRL::ComPtr<ID3D12Device9> device;
    D3D12_ASSERT_RETURN_NULL(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
    SET_D3D12_RESOURCE_DEBUG_NAME(device.Get(), "D3D12RenderDevice::device_");

#if _DEBUG
    if (isWindowsDeveloperModeEnabled())
    {
        device->SetStablePowerState(TRUE);
        PRINT_LOG("Developer Mode is enabled, setting stable power state for consistent GPU timings.\n");
    }
#endif

    D3D12_DESCRIPTOR_HEAP_DESC rtv_descriptor_heap_desc = {};
    rtv_descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_descriptor_heap_desc.NumDescriptors = kD3D12RTVDescriptorCount;
    rtv_descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_descriptor_heap_desc.NodeMask = 0;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap;
    D3D12_ASSERT_RETURN_NULL(device->CreateDescriptorHeap(&rtv_descriptor_heap_desc, IID_PPV_ARGS(&rtv_heap)));
    SET_D3D12_RESOURCE_DEBUG_NAME(rtv_heap.Get(), "D3D12RenderDevice::rtv_heap_");

    D3D12_DESCRIPTOR_HEAP_DESC dsv_descriptor_heap_desc = {};
    dsv_descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_descriptor_heap_desc.NumDescriptors = 1;
    dsv_descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsv_descriptor_heap_desc.NodeMask = 0;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap;
    D3D12_ASSERT_RETURN_NULL(device->CreateDescriptorHeap(&dsv_descriptor_heap_desc, IID_PPV_ARGS(&dsv_heap)));
    SET_D3D12_RESOURCE_DEBUG_NAME(dsv_heap.Get(), "D3D12RenderDevice::dsv_heap_");

    std::unique_ptr<D3D12RenderDevice> render_device(new D3D12RenderDevice());
    render_device->render_info_.is_ray_tracing_support = isDirectXRaytracingSupport(device.Get());

    render_device->rtv_heap_ = rtv_heap.Detach();
    render_device->dsv_heap_ = dsv_heap.Detach();
    render_device->device_ = device.Detach();
    render_device->adapter_ = adapter.Detach();
    render_device->dxgi_factory_ = dxgi_factory.Detach();

    return render_device;
}


std::unique_ptr<ISwapChain> D3D12RenderDevice::createSwapChain(const platform::IWindow& window)
{
    return D3D12SwapChain::create(*this, window);
}


D3D12RenderDevice::D3D12RenderDevice()
    : IRenderDevice()
    , dxgi_factory_(nullptr)
    , adapter_(nullptr)
    , device_(nullptr)
    , rtv_heap_(nullptr)
    , dsv_heap_(nullptr)
    , srv_heap_(nullptr)
{
}


D3D12RenderDevice::~D3D12RenderDevice()
{
    D3D12_SAFE_RELEASE(srv_heap_);
    D3D12_SAFE_RELEASE(dsv_heap_);
    D3D12_SAFE_RELEASE(rtv_heap_);

#if defined(_DEBUG)
    ID3D12DebugDevice* debug_layer = nullptr;
    if (SUCCEEDED(device_->QueryInterface(&debug_layer)))
    {
        debug_layer->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
        debug_layer->Release();
    }
#endif

    D3D12_SAFE_RELEASE(device_);
    D3D12_SAFE_RELEASE(adapter_);
    D3D12_SAFE_RELEASE(dxgi_factory_);
}

}  // namespace render
