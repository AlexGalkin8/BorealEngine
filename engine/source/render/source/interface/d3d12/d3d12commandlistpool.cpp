#include "d3d12commandlistpool.hpp"
#include "d3d12commandlist.hpp"

#include <cstdlib>

namespace render {

template<ECommandListType kType>
D3D12CommandList& D3D12CommandListPool<kType>::acquire(const std::string& name)
{
	if (!executing_command_lists_.empty())
	{
		if (executing_command_lists_.front().get().fence_value_ <= fence_.getCompletedValue())
		{
			D3D12CommandList& command_list = executing_command_lists_.front().get();
			executing_command_lists_.pop();
			command_list.setName(name);
			return command_list;
		}
	}

	std::unique_ptr<D3D12CommandList> new_list = D3D12CommandList::create(render_device_, kType);
	ASSERT_MSG(new_list, "Failed to create D3D12CommandList");

	new_list->setName(name);
	command_lists_.push_back(std::move(new_list));

	return *command_lists_.back();
}


template<ECommandListType kType>
void D3D12CommandListPool<kType>::release(std::span<const std::reference_wrapper<D3D12CommandList>> command_lists, FenceValue fence_value)
{
	for (const std::reference_wrapper<D3D12CommandList>& command_list_ref : command_lists)
	{
		D3D12CommandList& command_list = command_list_ref.get();
		command_list.fence_value_ = fence_value;
		executing_command_lists_.push(command_list);
	}
}


template<ECommandListType kType>
D3D12CommandListPool<kType>::D3D12CommandListPool(IRenderDevice& render_device, D3D12Fence& fence)
	: render_device_(render_device)
	, fence_(fence)
{
}

template class D3D12CommandListPool<ECommandListType::kDirect>;
template class D3D12CommandListPool<ECommandListType::kCompute>;
template class D3D12CommandListPool<ECommandListType::kCopy>;

}  // namespace render