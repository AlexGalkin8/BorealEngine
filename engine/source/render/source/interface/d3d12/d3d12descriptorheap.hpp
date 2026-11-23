#pragma once

#include "d3d12common.hpp"

#include <engine/core/utility/utility.h>
#include <engine/render/interface/common.h>

#include <array>
#include <cstdint>
#include <queue>
#include <mutex>
#include <memory>
#include <span>
#include <vector>


struct ID3D12DescriptorHeap;
struct ID3D12Device;


namespace render {

class D3D12DescriptorHeap final : public utility::NonCopyable
{
public:
	struct Allocation
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpu = {};
		uint32_t index = UINT32_MAX;

		bool isValid() const { return index != UINT32_MAX; }
	};

	static std::unique_ptr<D3D12DescriptorHeap> create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity);

	D3D12DescriptorHeap() = default;
	D3D12DescriptorHeap(D3D12DescriptorHeap&& other) noexcept;
	D3D12DescriptorHeap& operator=(D3D12DescriptorHeap&& other) noexcept;
	~D3D12DescriptorHeap();

	Allocation allocate();
	void free(uint32_t descriptor_index);
	void free(D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle);
	void reset();

	ID3D12DescriptorHeap* getHeap() const { return heap_; }
	uint32_t getDescriptorSize() const { return descriptor_size_; }
	uint32_t getCapacity() const { return heap_capacity_; }

	D3D12_CPU_DESCRIPTOR_HANDLE cpuAt(uint32_t descriptor_index) const;

private:
	D3D12DescriptorHeap(ID3D12DescriptorHeap* heap, uint32_t capacity, uint32_t descriptor_size);

private:
	ID3D12DescriptorHeap* heap_ = nullptr;
	uint32_t heap_capacity_ = 0;
	uint32_t descriptor_size_ = 0;

	std::queue<uint32_t> free_descriptors_idx_;
	uint32_t last_descriptor_idx_ = 0;
	std::mutex mutex_;
};


class D3D12ShaderVisibleDescriptorHeap final : public utility::NonCopyable
{
public:
	struct Range
	{
		uint32_t index = UINT32_MAX;
		uint32_t count = 0;
	};

	struct Allocation
	{
		Range range;
		D3D12_CPU_DESCRIPTOR_HANDLE cpu = {};
		D3D12_GPU_DESCRIPTOR_HANDLE gpu = {};

		bool isValid() const { return range.count != 0; }
	};

	static std::unique_ptr<D3D12ShaderVisibleDescriptorHeap> create(
		ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity);

	Allocation allocate(uint32_t count);
	void retire(std::span<const Range> ranges, ECommandListType queue, FenceValue fence_value);
	void reclaim(ECommandListType queue, FenceValue completed_fence_value);

	ID3D12DescriptorHeap* getHeap() const { return heap_.Get(); }
	uint32_t getDescriptorSize() const { return descriptor_size_; }
	uint32_t getCapacity() const { return capacity_; }

private:
	struct RetiredRange
	{
		Range range;
		FenceValue fence_value;
	};

	D3D12ShaderVisibleDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>&& heap, uint32_t capacity, uint32_t descriptor_size);
	void releaseRange(const Range& range);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
	uint32_t capacity_;
	uint32_t descriptor_size_;
	std::vector<Range> free_ranges_;
	std::array<std::queue<RetiredRange>, static_cast<size_t>(ECommandListType::kCopy) + 1> retired_ranges_;
	std::mutex mutex_;
};


class D3D12OwnedDescriptor final : public utility::NonCopyable
{
public:
	D3D12OwnedDescriptor() = default;
	D3D12OwnedDescriptor(D3D12DescriptorHeap* heap, D3D12DescriptorHeap::Allocation allocation)
		: heap_(heap)
		, allocation_(allocation)
	{
	}

	D3D12OwnedDescriptor(D3D12OwnedDescriptor&& other) noexcept
		: heap_(other.heap_)
		, allocation_(other.allocation_)
	{
		other.heap_ = nullptr;
		other.allocation_ = {};
	}

	D3D12OwnedDescriptor& operator=(D3D12OwnedDescriptor&& other) noexcept
	{
		if (this != &other)
		{
			reset();
			heap_ = other.heap_;
			allocation_ = other.allocation_;
			other.heap_ = nullptr;
			other.allocation_ = {};
		}
		return *this;
	}

	~D3D12OwnedDescriptor()
	{
		reset();
	}

	void reset()
	{
		if (heap_ != nullptr && allocation_.isValid())
		{
			heap_->free(allocation_.index);
		}
		heap_ = nullptr;
		allocation_ = {};
	}

	void reset(D3D12DescriptorHeap* heap, D3D12DescriptorHeap::Allocation allocation)
	{
		reset();
		heap_ = heap;
		allocation_ = allocation;
	}

	bool isValid() const { return heap_ != nullptr && allocation_.isValid(); }
	D3D12DescriptorHeap* getHeap() const { return heap_; }
	const D3D12DescriptorHeap::Allocation& getAllocation() const { return allocation_; }
	D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle() const { return allocation_.cpu; }

private:
	D3D12DescriptorHeap* heap_ = nullptr;
	D3D12DescriptorHeap::Allocation allocation_ = {};
};


}  // namespace render
