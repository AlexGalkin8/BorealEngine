#pragma once

#include <engine/core/utility/utility.h>

#include <d3d12.h>

#include <cstdint>
#include <queue>
#include <mutex>
#include <memory>


struct ID3D12DescriptorHeap;
struct ID3D12Device;


namespace render {

// Generic fence-driven ring buffer for transient per-frame allocations.
template<typename TIndex = uint32_t>
class RingBufferBase
{
protected:
	RingBufferBase() = default;
	explicit RingBufferBase(TIndex capacity)
		: capacity_(capacity)
		, start_(0)
		, end_(0)
		, is_full_(false)
	{
	}

	void moveStateFrom(RingBufferBase& other)
	{
		capacity_ = other.capacity_;
		start_ = other.start_;
		end_ = other.end_;
		is_full_ = other.is_full_;
		pending_frames_ = std::move(other.pending_frames_);

		other.capacity_ = 0;
		other.start_ = 0;
		other.end_ = 0;
		other.is_full_ = false;
	}

	void addPendingFrame(uint64_t fence_value)
	{
		pending_frames_.push({ fence_value, end_ });
	}

	void freeCompleted(uint64_t completed_fence_value)
	{
		while (!pending_frames_.empty() && pending_frames_.front().fence_value <= completed_fence_value)
		{
			start_ = pending_frames_.front().end;
			pending_frames_.pop();
			is_full_ = false;
		}

		if (start_ == end_ && !is_full_)
		{
			start_ = 0;
			end_ = 0;
		}
	}

	bool tryAllocate(TIndex* out_offset, TIndex allocation_size, TIndex align = 1)
	{
		if (out_offset == nullptr || allocation_size == 0 || allocation_size > capacity_)
		{
			return false;
		}
		if (is_full_)
		{
			return false;
		}

		TIndex aligned_end = (align > 1) ? ((end_ + (align - 1)) / align) * align : end_;

		if (start_ <= aligned_end)
		{
			if (aligned_end > capacity_ || capacity_ - aligned_end < allocation_size)
			{
				if (start_ == 0)
				{
					return false;
				}
				aligned_end = 0;
			}
		}

		if (start_ > aligned_end)
		{
			if (start_ - aligned_end < allocation_size)
			{
				return false;
			}
		}

		*out_offset = aligned_end;
		end_ = (aligned_end + allocation_size) % capacity_;
		is_full_ = (start_ == end_);
		return true;
	}

	void resetRing()
	{
		while (!pending_frames_.empty())
		{
			pending_frames_.pop();
		}
		start_ = 0;
		end_ = 0;
		is_full_ = false;
	}

	TIndex getCapacityValue() const { return capacity_; }

private:
	struct PendingFrame
	{
		uint64_t fence_value = 0;
		TIndex end = 0;
	};

	TIndex capacity_ = 0;
	TIndex start_ = 0;
	TIndex end_ = 0;
	bool is_full_ = false;
	std::queue<PendingFrame> pending_frames_;
};


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


class D3D12DescriptorRingBuffer final : public utility::NonCopyable, protected RingBufferBase<uint32_t>
{
public:
	struct Allocation
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpu = {};
		D3D12_GPU_DESCRIPTOR_HANDLE gpu = {};
		uint32_t index = UINT32_MAX;

		bool isValid() const { return index != UINT32_MAX; }
	};

	static std::unique_ptr<D3D12DescriptorRingBuffer> create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity);

	D3D12DescriptorRingBuffer() = default;
	D3D12DescriptorRingBuffer(D3D12DescriptorRingBuffer&& other) noexcept;
	D3D12DescriptorRingBuffer& operator=(D3D12DescriptorRingBuffer&& other) noexcept;
	~D3D12DescriptorRingBuffer();

	Allocation allocate();
	Allocation allocateContiguous(uint32_t count);

	void addPendingFrame(uint64_t fence_value);
	void freeCompleted(uint64_t completed_fence_value);
	void beginFrame(uint32_t frame_index, uint32_t frame_count);

	void reset();

	ID3D12DescriptorHeap* getHeap() const { return heap_; }
	uint32_t getDescriptorSize() const { return descriptor_size_; }
	uint32_t getCapacity() const { return heap_capacity_; }

	D3D12_CPU_DESCRIPTOR_HANDLE cpuAt(uint32_t descriptor_index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuAt(uint32_t descriptor_index) const;

private:
	D3D12DescriptorRingBuffer(ID3D12DescriptorHeap* heap, uint32_t capacity, uint32_t descriptor_size);

private:
	ID3D12DescriptorHeap* heap_ = nullptr;
	uint32_t heap_capacity_ = 0;
	uint32_t descriptor_size_ = 0;
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


struct D3D12DescriptorRange final : utility::NonCopyable
{
	D3D12DescriptorRingBuffer::Allocation allocation = {};
	uint32_t capacity = 0;
	uint32_t offset = 0;

	bool isValid() const { return allocation.isValid(); }
	void reset()
	{
		allocation = {};
		capacity = 0;
		offset = 0;
	}
};

}  // namespace render
