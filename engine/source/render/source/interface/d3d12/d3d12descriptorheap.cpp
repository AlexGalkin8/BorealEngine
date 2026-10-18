#include "d3d12descriptorheap.hpp"
#include "d3d12common.hpp"

#include <engine/core/utility/assert.h>


namespace render {

namespace {

D3D12_CPU_DESCRIPTOR_HANDLE makeCpuHandle(ID3D12DescriptorHeap* heap, uint32_t descriptor_size, uint32_t descriptor_index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(descriptor_index) * descriptor_size;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE makeGpuHandle(ID3D12DescriptorHeap* heap, uint32_t descriptor_size, uint32_t descriptor_index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = heap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<UINT64>(descriptor_index) * descriptor_size;
	return handle;
}

}

std::unique_ptr<D3D12DescriptorHeap> D3D12DescriptorHeap::create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.NumDescriptors = capacity;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;

	ID3D12DescriptorHeap* heap = nullptr;
	D3D12_ASSERT_RETURN_NULL(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

	const uint32_t descriptor_size = device->GetDescriptorHandleIncrementSize(type);
	return std::unique_ptr<D3D12DescriptorHeap>(new D3D12DescriptorHeap(heap, capacity, descriptor_size));
}


D3D12DescriptorHeap::D3D12DescriptorHeap(ID3D12DescriptorHeap* heap, uint32_t capacity, uint32_t descriptor_size)
	: heap_(heap)
	, heap_capacity_(capacity)
	, descriptor_size_(descriptor_size)
	, last_descriptor_idx_(0)
{
}


D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12DescriptorHeap&& other) noexcept
	: heap_(other.heap_)
	, heap_capacity_(other.heap_capacity_)
	, descriptor_size_(other.descriptor_size_)
	, free_descriptors_idx_(std::move(other.free_descriptors_idx_))
	, last_descriptor_idx_(other.last_descriptor_idx_)
{
	other.heap_ = nullptr;
	other.heap_capacity_ = 0;
	other.descriptor_size_ = 0;
	other.last_descriptor_idx_ = 0;
}


D3D12DescriptorHeap& D3D12DescriptorHeap::operator=(D3D12DescriptorHeap&& other) noexcept
{
	if (this != &other)
	{
		D3D12_SAFE_RELEASE(heap_);

		heap_ = other.heap_;
		heap_capacity_ = other.heap_capacity_;
		descriptor_size_ = other.descriptor_size_;
		free_descriptors_idx_ = std::move(other.free_descriptors_idx_);
		last_descriptor_idx_ = other.last_descriptor_idx_;

		other.heap_ = nullptr;
		other.heap_capacity_ = 0;
		other.descriptor_size_ = 0;
		other.last_descriptor_idx_ = 0;
	}
	return *this;
}


D3D12DescriptorHeap::~D3D12DescriptorHeap()
{
	D3D12_SAFE_RELEASE(heap_);
}


D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::cpuAt(uint32_t descriptor_index) const
{
	if (descriptor_index >= heap_capacity_)
	{
		return {};
	}

	return makeCpuHandle(heap_, descriptor_size_, descriptor_index);
}


D3D12DescriptorHeap::Allocation D3D12DescriptorHeap::allocate()
{
	std::lock_guard<std::mutex> lock(mutex_);

	uint32_t descriptor_index = UINT32_MAX;
	if (!free_descriptors_idx_.empty())
	{
		descriptor_index = free_descriptors_idx_.front();
		free_descriptors_idx_.pop();
	}
	else
	{
		ASSERT_MSG_RETURN(last_descriptor_idx_ < heap_capacity_, Allocation{}, "D3D12DescriptorHeap: pool heap is out of descriptors");
		descriptor_index = last_descriptor_idx_++;
	}

	Allocation alloc{};
	alloc.index = descriptor_index;
	alloc.cpu = cpuAt(descriptor_index);
	return alloc;
}


void D3D12DescriptorHeap::free(uint32_t descriptor_index)
{
	ASSERT_MSG_RETURN_VOID(descriptor_index < heap_capacity_, "D3D12DescriptorHeap::free(): descriptor index out of range");

	std::lock_guard<std::mutex> lock(mutex_);
	free_descriptors_idx_.push(descriptor_index);
}


void D3D12DescriptorHeap::free(D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle)
{
	const D3D12_CPU_DESCRIPTOR_HANDLE first_handle = cpuAt(0);
	ASSERT_MSG_RETURN_VOID(descriptor_handle.ptr >= first_handle.ptr, "D3D12DescriptorHeap::free(handle): handle is out of heap range");

	const uint64_t delta = descriptor_handle.ptr - first_handle.ptr;
	ASSERT_MSG_RETURN_VOID(delta % descriptor_size_ == 0, "D3D12DescriptorHeap::free(handle): invalid descriptor handle alignment");

	const uint32_t descriptor_index = static_cast<uint32_t>(delta / descriptor_size_);
	free(descriptor_index);
}


void D3D12DescriptorHeap::reset()
{
	std::lock_guard<std::mutex> lock(mutex_);
	while (!free_descriptors_idx_.empty())
	{
		free_descriptors_idx_.pop();
	}
	last_descriptor_idx_ = 0;
}


std::unique_ptr<D3D12DescriptorRingBuffer> D3D12DescriptorRingBuffer::create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.NumDescriptors = capacity;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	D3D12_ASSERT_RETURN_NULL(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

	const uint32_t descriptor_size = device->GetDescriptorHandleIncrementSize(type);
	return std::unique_ptr<D3D12DescriptorRingBuffer>(new D3D12DescriptorRingBuffer(heap.Detach(), capacity, descriptor_size));
}


D3D12DescriptorRingBuffer::D3D12DescriptorRingBuffer(ID3D12DescriptorHeap* heap, uint32_t capacity, uint32_t descriptor_size)
	: RingBufferBase<uint32_t>(capacity)
	, heap_(heap)
	, heap_capacity_(capacity)
	, descriptor_size_(descriptor_size)
{
}


D3D12DescriptorRingBuffer::D3D12DescriptorRingBuffer(D3D12DescriptorRingBuffer&& other) noexcept
	: RingBufferBase<uint32_t>()
	, heap_(other.heap_)
	, heap_capacity_(other.heap_capacity_)
	, descriptor_size_(other.descriptor_size_)
{
	moveStateFrom(other);
	other.heap_ = nullptr;
	other.heap_capacity_ = 0;
	other.descriptor_size_ = 0;
}


D3D12DescriptorRingBuffer& D3D12DescriptorRingBuffer::operator=(D3D12DescriptorRingBuffer&& other) noexcept
{
	if (this != &other)
	{
		D3D12_SAFE_RELEASE(heap_);

		heap_ = other.heap_;
		heap_capacity_ = other.heap_capacity_;
		descriptor_size_ = other.descriptor_size_;

		moveStateFrom(other);
		other.heap_ = nullptr;
		other.heap_capacity_ = 0;
		other.descriptor_size_ = 0;
	}
	return *this;
}


D3D12DescriptorRingBuffer::~D3D12DescriptorRingBuffer()
{
	D3D12_SAFE_RELEASE(heap_);
}


D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorRingBuffer::cpuAt(uint32_t descriptor_index) const
{
	if (descriptor_index >= heap_capacity_)
	{
		return {};
	}

	return makeCpuHandle(heap_, descriptor_size_, descriptor_index);
}


D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorRingBuffer::gpuAt(uint32_t descriptor_index) const
{
	if (descriptor_index >= heap_capacity_)
	{
		return {};
	}

	return makeGpuHandle(heap_, descriptor_size_, descriptor_index);
}


D3D12DescriptorRingBuffer::Allocation D3D12DescriptorRingBuffer::allocate()
{
	uint32_t descriptor_index = UINT32_MAX;
	ASSERT_MSG_RETURN(tryAllocate(&descriptor_index, 1, 1), Allocation{}, "D3D12DescriptorRingBuffer: ring heap is out of descriptors");

	Allocation alloc{};
	alloc.index = descriptor_index;
	alloc.cpu = cpuAt(descriptor_index);
	alloc.gpu = gpuAt(descriptor_index);
	return alloc;
}


D3D12DescriptorRingBuffer::Allocation D3D12DescriptorRingBuffer::allocateContiguous(uint32_t count)
{
	ASSERT_MSG_RETURN(count > 0, Allocation{}, "D3D12DescriptorRingBuffer::allocateContiguous(): count must be > 0");

	uint32_t descriptor_index = UINT32_MAX;
	ASSERT_MSG_RETURN(tryAllocate(&descriptor_index, count, 1), Allocation{}, "D3D12DescriptorRingBuffer: not enough contiguous space in ring buffer");

	Allocation alloc{};
	alloc.index = descriptor_index;
	alloc.cpu = cpuAt(descriptor_index);
	alloc.gpu = gpuAt(descriptor_index);
	return alloc;
}


void D3D12DescriptorRingBuffer::addPendingFrame(uint64_t fence_value)
{
	RingBufferBase<uint32_t>::addPendingFrame(fence_value);
}


void D3D12DescriptorRingBuffer::freeCompleted(uint64_t completed_fence_value)
{
	RingBufferBase<uint32_t>::freeCompleted(completed_fence_value);
}


void D3D12DescriptorRingBuffer::beginFrame(uint32_t frame_index, uint32_t frame_count)
{
	(void)frame_index;
	(void)frame_count;
}


void D3D12DescriptorRingBuffer::reset()
{
	resetRing();
}

}  // namespace render
