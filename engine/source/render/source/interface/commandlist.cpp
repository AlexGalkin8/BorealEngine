#include "interface/commandlist.h"


namespace render {

ICommandList::~ICommandList()
{

}


ICommandList::ICommandList(IRenderDevice& render_device, ECommandListType type)
	: render_device_(render_device)
	, is_recording_(false)
	, type_(type)
{
}

}  // namespace render
