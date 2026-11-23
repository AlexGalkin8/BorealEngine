#include "d3d12descriptorheap.hpp"
#include "d3d12common.hpp"

#include <engine/core/utility/assert.h>

#include <algorithm>
#include <iterator>


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


std::unique_ptr<D3D12ShaderVisibleDescriptorHeap> D3D12ShaderVisibleDescriptorHeap::create(
	ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.NumDescriptors = capacity;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	D3D12_ASSERT_RETURN_NULL(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

	const uint32_t descriptor_size = device->GetDescriptorHandleIncrementSize(type);
	return std::unique_ptr<D3D12ShaderVisibleDescriptorHeap>(
		new D3D12ShaderVisibleDescriptorHeap(std::move(heap), capacity, descriptor_size));
}


D3D12ShaderVisibleDescriptorHeap::Allocation D3D12ShaderVisibleDescriptorHeap::allocate(uint32_t count)
{
	ASSERT_MSG_RETURN_EMPTY(count > 0, "A descriptor table must contain at least one descriptor");
	const std::lock_guard<std::mutex> lock(mutex_);
	auto range_it = std::find_if(free_ranges_.begin(), free_ranges_.end(), [count](const Range& range)
	{
		return range.count >= count;
	});
	if (range_it == free_ranges_.end())
	{
		return {};
	}

	const uint32_t index = range_it->index;
	range_it->index += count;
	range_it->count -= count;
	if (range_it->count == 0)
	{
		free_ranges_.erase(range_it);
	}

	return { { index, count }, makeCpuHandle(heap_.Get(), descriptor_size_, index),
		makeGpuHandle(heap_.Get(), descriptor_size_, index) };
}


void D3D12ShaderVisibleDescriptorHeap::retire(std::span<const Range> ranges, ECommandListType queue, FenceValue fence_value)
{
	if (ranges.empty())
	{
		return;
	}

	const std::lock_guard<std::mutex> lock(mutex_);
	std::queue<RetiredRange>& retired = retired_ranges_[static_cast<size_t>(queue)];
	for (const Range& range : ranges)
	{
		retired.push({ range, fence_value });
	}
}


void D3D12ShaderVisibleDescriptorHeap::reclaim(ECommandListType queue, FenceValue completed_fence_value)
{
	// A removed device reports UINT64_MAX and cannot prove that work completed.
	if (completed_fence_value == UINT64_MAX)
	{
		return;
	}

	const std::lock_guard<std::mutex> lock(mutex_);
	std::queue<RetiredRange>& retired = retired_ranges_[static_cast<size_t>(queue)];
	while (!retired.empty() && retired.front().fence_value <= completed_fence_value)
	{
		releaseRange(retired.front().range);
		retired.pop();
	}
}


D3D12ShaderVisibleDescriptorHeap::D3D12ShaderVisibleDescriptorHeap(
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>&& heap, uint32_t capacity, uint32_t descriptor_size)
	: heap_(std::move(heap))
	, capacity_(capacity)
	, descriptor_size_(descriptor_size)
	, free_ranges_({ { 0, capacity } })
{}


void D3D12ShaderVisibleDescriptorHeap::releaseRange(const Range& range)
{
	auto next = std::lower_bound(free_ranges_.begin(), free_ranges_.end(), range.index,
		[](const Range& free_range, uint32_t index) { return free_range.index < index; });

	Range merged = range;
	if (next != free_ranges_.begin())
	{
		const auto previous = std::prev(next);
		if (previous->index + previous->count == merged.index)
		{
			merged.index = previous->index;
			merged.count += previous->count;
			next = free_ranges_.erase(previous);
		}
	}
	if (next != free_ranges_.end() && merged.index + merged.count == next->index)
	{
		merged.count += next->count;
		next = free_ranges_.erase(next);
	}
	free_ranges_.insert(next, merged);
}

}  // namespace render
