#pragma once

#include "interface/devicecontext.h"

#include <memory>


namespace render {
class IRenderDevice;
}  // namespace render


namespace render {

class IDeviceContext;

template<ERenderAPI>
std::unique_ptr<IDeviceContext> createDeviceContext(IRenderDevice&) = delete;

}  // namespace render
