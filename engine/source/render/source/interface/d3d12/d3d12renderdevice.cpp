#include "d3d12renderdevice.hpp"
#include "d3d12common.hpp"
#include "d3d12swapchain.hpp"
#include "d3d12fence.hpp"
#include "d3d12descriptorheap.hpp"
#include "d3d12commandqueue.hpp"
#include "d3d12commandlist.hpp"
#include "d3d12texture.hpp"
#include "d3d12buffer.hpp"

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

	std::unique_ptr<D3D12RenderDevice> render_device(new D3D12RenderDevice());
	render_device->render_info_.is_ray_tracing_support = isDirectXRaytracingSupport(device.Get());

	render_device->dxgi_factory_ = dxgi_factory.Detach();
	render_device->adapter_ = adapter.Detach();
	render_device->device_ = device.Detach();

	render_device->rtv_descriptor_heap_ = D3D12DescriptorHeap::create(
		render_device->device_,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		kD3D12RTVDescriptorCount);

	render_device->dsv_descriptor_heap_ = D3D12DescriptorHeap::create(
		render_device->device_,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		1);

	render_device->cbv_srv_uav_cpu_descriptor_heap_ = D3D12DescriptorHeap::create(
		render_device->device_,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		kD3D12SRVDescriptorCount);

	render_device->cbv_srv_uav_descriptor_heap_ = D3D12DescriptorRingBuffer::create(
		render_device->device_,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		kD3D12SRVDescriptorCount);

	render_device->direct_queue_  = D3D12CommandQueue<ECommandListType::kDirect>::create(*render_device);
	ASSERT_MSG_RETURN_NULL(render_device->direct_queue_ != nullptr, "D3D12RenderDevice: failed to create direct queue wrapper");

	render_device->compute_queue_ = D3D12CommandQueue<ECommandListType::kCompute>::create(*render_device);
	ASSERT_MSG_RETURN_NULL(render_device->compute_queue_ != nullptr, "D3D12RenderDevice: failed to create compute queue wrapper");

	render_device->copy_queue_    = D3D12CommandQueue<ECommandListType::kCopy>::create(*render_device);
	ASSERT_MSG_RETURN_NULL(render_device->copy_queue_ != nullptr,    "D3D12RenderDevice: failed to create copy queue wrapper");

	return render_device;
}


std::unique_ptr<ISwapChain> D3D12RenderDevice::createSwapChain(const platform::IWindow& window)
{
	return D3D12SwapChain::create(*this, window);
}


std::unique_ptr<ITexture> D3D12RenderDevice::createTexture(const ITexture::Desc& desc)
{
	return D3D12Texture::create(*this, desc);
}


std::unique_ptr<IBuffer> D3D12RenderDevice::createBuffer(const IBuffer::Desc& desc)
{
	return D3D12Buffer::create(*this, desc);
}


ICommandList& D3D12RenderDevice::acquireCommandList(ECommandListType type, const std::string& name)
{
	D3D12CommandList* command_list = nullptr;

	switch (type)
	{
	case ECommandListType::kDirect:
		command_list = &static_cast<D3D12CommandList&>(direct_queue_->acquireCommandList(name));
		break;
	case ECommandListType::kCompute:
		command_list = &static_cast<D3D12CommandList&>(compute_queue_->acquireCommandList(name));
		break;
	case ECommandListType::kCopy:
		command_list = &static_cast<D3D12CommandList&>(copy_queue_->acquireCommandList(name));
		break;
	default:
		ASSERT_FAIL("Invalid command list type");
		command_list = &static_cast<D3D12CommandList&>(direct_queue_->acquireCommandList(name));
		break;
	}

	D3D12CommandList::DescriptorRange descriptor_range = {};
	descriptor_range.allocation = getCbvSrvUavDescriptorHeap()->allocateContiguous(kCommandListReservedDescriptorCount);
	ASSERT_MSG_RETURN(descriptor_range.isValid(), *command_list, "D3D12RenderDevice: failed to reserve descriptor range for command list");
	descriptor_range.capacity = kCommandListReservedDescriptorCount;
	descriptor_range.offset = 0;

	command_list->beginRecording(descriptor_range);
	return *command_list;
}


void D3D12RenderDevice::submitCommandList(ICommandList& command_list)
{
	D3D12CommandList& d3d12_command_list = static_cast<D3D12CommandList&>(command_list);
	d3d12_command_list.endRecording();

	switch (d3d12_command_list.getType())
	{
	case ECommandListType::kDirect:
		direct_queue_->addPendingCommandList(command_list);
		break;
	case ECommandListType::kCompute:
		compute_queue_->addPendingCommandList(command_list);
		break;
	case ECommandListType::kCopy:
		copy_queue_->addPendingCommandList(command_list);
		break;
	default:
		ASSERT_FAIL("Invalid command list type");
		break;
	}
}

void D3D12RenderDevice::executeCommandLists(ECommandListType type)
{
	switch (type)
	{
	case ECommandListType::kDirect:
	{
		const FenceValue fence_value = direct_queue_->executePendingCommandLists();
		if (fence_value > 0)
		{
			cbv_srv_uav_descriptor_heap_->addPendingFrame(fence_value);
			cbv_srv_uav_descriptor_heap_->freeCompleted(direct_queue_->getD3D12Fence().getCompletedValue());
		}
		break;
	}
	case ECommandListType::kCompute:
	{
		const FenceValue fence_value = compute_queue_->executePendingCommandLists();
		if (fence_value > 0)
		{
			cbv_srv_uav_descriptor_heap_->addPendingFrame(fence_value);
			cbv_srv_uav_descriptor_heap_->freeCompleted(compute_queue_->getD3D12Fence().getCompletedValue());
		}
		break;
	}
	case ECommandListType::kCopy:
	{
		const FenceValue fence_value = copy_queue_->executePendingCommandLists();
		if (fence_value > 0)
		{
			cbv_srv_uav_descriptor_heap_->addPendingFrame(fence_value);
			cbv_srv_uav_descriptor_heap_->freeCompleted(copy_queue_->getD3D12Fence().getCompletedValue());
		}
		break;
	}
	}
}


void D3D12RenderDevice::waitGpuIdle()
{
	direct_queue_->waitGpuIdle();
	compute_queue_->waitGpuIdle();
	copy_queue_->waitGpuIdle();
}


D3D12RenderDevice::D3D12RenderDevice()
	: IRenderDevice()
	, dxgi_factory_(nullptr)
	, adapter_(nullptr)
	, device_(nullptr)
{
}


D3D12RenderDevice::~D3D12RenderDevice()
{
#if defined(_DEBUG)
	ID3D12DebugDevice* debug_layer = nullptr;
	if (SUCCEEDED(device_->QueryInterface(&debug_layer)))
	{
		debug_layer->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		debug_layer->Release();
	}
#endif

	cbv_srv_uav_descriptor_heap_.reset();
	cbv_srv_uav_cpu_descriptor_heap_.reset();
	dsv_descriptor_heap_.reset();
	rtv_descriptor_heap_.reset();

	D3D12_SAFE_RELEASE(device_);
	D3D12_SAFE_RELEASE(adapter_);
	D3D12_SAFE_RELEASE(dxgi_factory_);
}

}  // namespace render
