#include "d3d12renderinterface.hpp"
#include "d3d12common.hpp"
#include "d3d12swapchain.hpp"
#include "d3d12pipelinestate.hpp"
#include "d3d12fence.hpp"

#include <cstdint>


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
			80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
		};
		DXGI_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(hide);
		filter.DenyList.pIDList = hide;
		dxgi_info_queue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
	}
}


static bool isWindowsDeveloperModeEnabled()
{
	bool developer_mode_enabled = false;
	// Look in the Windows Registry to determine if Developer Mode is enabled
	HKEY h_key;
	LSTATUS result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_READ, &h_key);
	if (result == ERROR_SUCCESS)
	{
		DWORD keyValue, keySize = sizeof(DWORD);
		result = RegQueryValueEx(h_key, L"AllowDevelopmentWithoutDevLicense", 0, NULL, (byte*)&keyValue, &keySize);
		if (result == ERROR_SUCCESS && keyValue == 1)
			developer_mode_enabled = true;
		RegCloseKey(h_key);
	}
	return true;
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


std::unique_ptr<D3D12RenderInterface> D3D12RenderInterface::create(const CreationDesc& creation_desc)
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
	SET_D3D12_RESOURCE_DEBUG_NAME(device.Get(), "D3D12RenderInterface::device_");

#if _DEBUG
	// Prevent the GPU from overclocking or underclocking to get consistent timings
	if (isWindowsDeveloperModeEnabled())
	{
		device->SetStablePowerState(TRUE);
		PRINT_LOG("Developer Mode is enabled, setting stable power state for consistent GPU timings.\n");
	}
#endif

	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> direct_queue;
	D3D12_ASSERT_RETURN_NULL(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&direct_queue)));
	SET_D3D12_RESOURCE_DEBUG_NAME(direct_queue.Get(), "D3D12RenderInterface::direct_queue_");

	D3D12_DESCRIPTOR_HEAP_DESC rtv_descriptor_heap_desc = {};
	rtv_descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_descriptor_heap_desc.NumDescriptors = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
	rtv_descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtv_descriptor_heap_desc.NodeMask = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_heap;
	D3D12_ASSERT_RETURN_NULL(device->CreateDescriptorHeap(&rtv_descriptor_heap_desc, IID_PPV_ARGS(&rtv_heap)));
	SET_D3D12_RESOURCE_DEBUG_NAME(rtv_heap.Get(), "D3D12RenderInterface::rtv_heap_");

	D3D12_DESCRIPTOR_HEAP_DESC dsv_descriptor_heap_desc = {};
	dsv_descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsv_descriptor_heap_desc.NumDescriptors = 1;
	dsv_descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsv_descriptor_heap_desc.NodeMask = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap;
	D3D12_ASSERT_RETURN_NULL(device->CreateDescriptorHeap(&dsv_descriptor_heap_desc, IID_PPV_ARGS(&dsv_heap)));
	SET_D3D12_RESOURCE_DEBUG_NAME(dsv_heap.Get(), "D3D12RenderInterface::dsv_heap_");

	std::array<FrameContext, kBackBufferCount> frame_contexts = {};
	for (FrameContext& frame_context : frame_contexts)
	{
		D3D12_ASSERT_RETURN_NULL(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame_context.command_allocator)));
		SET_D3D12_RESOURCE_DEBUG_NAME(frame_context.command_allocator, "D3D12RenderInterface::frame_context_::command_allocator");

		frame_context.fence_value = 0;
	}

	Microsoft::WRL::ComPtr<ID3D12CommandList> command_list;
	D3D12_ASSERT_RETURN_NULL(device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT, frame_contexts[0].command_allocator, nullptr, IID_PPV_ARGS(&command_list)));
	SET_D3D12_RESOURCE_DEBUG_NAME(command_list.Get(), "D3D12RenderInterface::command_list_");

	std::unique_ptr<D3D12RenderInterface> d3d12_render_interface = std::unique_ptr<D3D12RenderInterface>(new D3D12RenderInterface());
	d3d12_render_interface->render_info_.is_ray_tracing_support = isDirectXRaytracingSupport(device.Get());

	d3d12_render_interface->rtv_heap_ = rtv_heap.Detach();
	d3d12_render_interface->dsv_heap_ = dsv_heap.Detach();

	d3d12_render_interface->direct_queue_ = direct_queue.Detach();
	d3d12_render_interface->device_ = device.Detach();

	d3d12_render_interface->adapter_ = adapter.Detach();
	d3d12_render_interface->dxgi_factory_ = dxgi_factory.Detach();

	d3d12_render_interface->frame_contexts_ = std::move(frame_contexts);

	d3d12_render_interface->command_list_ = static_cast<ID3D12GraphicsCommandList*>(command_list.Detach());

	return d3d12_render_interface;
}

#pragma region interface_methods

std::unique_ptr<ISwapChain> D3D12RenderInterface::createSwapChain(const platform::IWindow& window)
{
	return D3D12SwapChain::create(*this, window);
}


void D3D12RenderInterface::beginFrame()
{
	IRenderInterface::beginFrame();

	FrameContext& current_frame_context = frame_contexts_[current_frame_index_];
	if (frame_fence_->getCompletedValue() < current_frame_context.fence_value)
	{
		frame_fence_->waitValueOnCpu(current_frame_context.fence_value);
	}

	D3D12_ASSERT(current_frame_context.command_allocator->Reset());
	D3D12_ASSERT(command_list_->Reset(current_frame_context.command_allocator, nullptr));

	//ID3D12DescriptorHeap* descriptorHeaps[] = { srv_heap_ };
	//command_list_->SetDescriptorHeaps(1, descriptorHeaps);
}


void D3D12RenderInterface::endFrame()
{
	ASSERT_MSG_RETURN_VOID(is_frame_started_, "Frame not started");

	D3D12_ASSERT(command_list_->Close());

	ID3D12CommandList* command_lists[] = { command_list_ };
	direct_queue_->ExecuteCommandLists(1, command_lists);
	static_cast<D3D12Fence*>(frame_fence_.get())->signalOnGpu(direct_queue_);

	frame_contexts_[current_frame_index_].fence_value = frame_fence_->getNextSignalValue();

	IRenderInterface::endFrame();
}


void D3D12RenderInterface::draw(uint32_t vertex_count, uint32_t vertex_start_offset)
{
	drawInstanced(vertex_count, 1, vertex_start_offset, 0);
}


void D3D12RenderInterface::drawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
	drawIndexedInstanced(index_count, 1, start_index_location, base_vertex_location, 0);
}


void D3D12RenderInterface::drawInstanced(uint32_t vertex_count_per_instance, uint32_t instance_count,
	uint32_t start_vertex_location, uint32_t start_instance_location)
{
	//FlushResourceBarriers();
	//m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	//m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	command_list_->DrawInstanced(vertex_count_per_instance, instance_count, start_vertex_location, start_instance_location);
}


void D3D12RenderInterface::drawIndexedInstanced(uint32_t index_count_per_instance, uint32_t instance_count, uint32_t start_index_location,
	int32_t base_vertex_location, uint32_t start_instance_location)
{
	//FlushResourceBarriers();
	//m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	//m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	command_list_->DrawIndexedInstanced(index_count_per_instance, instance_count, start_index_location, base_vertex_location, start_instance_location);
}


#pragma endregion interface_methods


D3D12RenderInterface::~D3D12RenderInterface()
{
	D3D12_SAFE_RELEASE(command_list_);

	for (FrameContext& frame_context : frame_contexts_)
	{
		D3D12_SAFE_RELEASE(frame_context.command_allocator);
	}

	D3D12_SAFE_RELEASE(srv_heap_);
	D3D12_SAFE_RELEASE(dsv_heap_);
	D3D12_SAFE_RELEASE(rtv_heap_);

	D3D12_SAFE_RELEASE(direct_queue_);

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


D3D12RenderInterface::D3D12RenderInterface()
	: IRenderInterface()
	, dxgi_factory_(nullptr)
	, adapter_(nullptr)
	, device_(nullptr)
	, direct_queue_(nullptr)
	, rtv_heap_(nullptr)
	, dsv_heap_(nullptr)
	, srv_heap_(nullptr)
	, command_list_(nullptr)
	, frame_contexts_()
{
}

}  // namespace render
