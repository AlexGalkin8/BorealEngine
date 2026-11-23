#include <engine/render/imguibackend.h>

#include "interface/commandlist.h"
#include "interface/gpuresourcemanager.h"
#include "interface/renderdevice.h"
#include "pipelinestatemanager.hpp"

#include <engine/core/utility/assert.h>
#include <engine/platform/input.h>

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <vector>


namespace render {

static void updateInput(const platform::IInput& input, ImGuiIO& io)
{
	using platform::EKey;
	float x = 0.0f;
	float y = 0.0f;
	const bool focused = input.getMousePosition(x, y);
	io.AddFocusEvent(focused);
	io.AddMousePosEvent(focused ? x : -FLT_MAX, focused ? y : -FLT_MAX);
	io.AddMouseButtonEvent(0, input.isKeyDown(EKey::kMouseLeft));
	io.AddMouseButtonEvent(1, input.isKeyDown(EKey::kMouseRight));
	io.AddMouseButtonEvent(2, input.isKeyDown(EKey::kMouseMiddle));
	io.AddMouseWheelEvent(0.0f, input.getMouseWheel());
	io.AddKeyEvent(ImGuiMod_Ctrl, input.isKeyDown(EKey::kLeftCtrl) || input.isKeyDown(EKey::kRightCtrl));
	io.AddKeyEvent(ImGuiMod_Shift, input.isKeyDown(EKey::kLeftShift) || input.isKeyDown(EKey::kRightShift));
	io.AddKeyEvent(ImGuiMod_Alt, input.isKeyDown(EKey::kLeftAlt) || input.isKeyDown(EKey::kRightAlt));

	struct KeyMapping
	{
		EKey engine_key;
		ImGuiKey imgui_key;
	};
	static constexpr KeyMapping kKeys[] = {
		{ EKey::kTab, ImGuiKey_Tab }, { EKey::kUp, ImGuiKey_UpArrow }, { EKey::kDown, ImGuiKey_DownArrow },
		{ EKey::kLeft, ImGuiKey_LeftArrow }, { EKey::kRight, ImGuiKey_RightArrow },
		{ EKey::kHome, ImGuiKey_Home }, { EKey::kEnd, ImGuiKey_End },
		{ EKey::kPageUp, ImGuiKey_PageUp }, { EKey::kPageDown, ImGuiKey_PageDown },
		{ EKey::kInsert, ImGuiKey_Insert }, { EKey::kDelete, ImGuiKey_Delete },
		{ EKey::kBackspace, ImGuiKey_Backspace }, { EKey::kSpace, ImGuiKey_Space },
		{ EKey::kReturn, ImGuiKey_Enter }, { EKey::kEscape, ImGuiKey_Escape },
	};
	for (const KeyMapping& key : kKeys)
	{
		io.AddKeyEvent(key.imgui_key, input.isKeyDown(key.engine_key));
	}
	const auto add_key_range = [&input, &io](EKey first, EKey last, ImGuiKey first_imgui_key)
	{
		const uint32_t first_key = static_cast<uint32_t>(first);
		const uint32_t last_key = static_cast<uint32_t>(last);
		for (uint32_t key = first_key; key <= last_key; ++key)
		{
			io.AddKeyEvent(static_cast<ImGuiKey>(first_imgui_key + key - first_key),
				input.isKeyDown(static_cast<EKey>(key)));
		}
	};
	add_key_range(EKey::kA, EKey::kZ, ImGuiKey_A);
	add_key_range(EKey::k0, EKey::k9, ImGuiKey_0);
	add_key_range(EKey::kF1, EKey::kF12, ImGuiKey_F1);
	for (char16_t character : input.getTextInput())
	{
		io.AddInputCharacterUTF16(character);
	}
}


std::unique_ptr<ImGuiBackend> ImGuiBackend::create(IRenderDevice& render_device)
{
	std::unique_ptr<ImGuiBackend> backend(new ImGuiBackend(render_device));
	if (!backend->init())
	{
		return nullptr;
	}
	return backend;
}


void ImGuiBackend::newFrame(const platform::IInput& input, uint32_t width, uint32_t height, float delta_time)
{
	ImGui::SetCurrentContext(context_);
	ImGuiIO& io = ImGui::GetIO();
	updateInput(input, io);
	io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
	io.DeltaTime = std::max(delta_time, 0.000001f);
	ImGui::NewFrame();
}


bool ImGuiBackend::wantsMouse() const
{
	return ImGui::GetIO().WantCaptureMouse;
}


bool ImGuiBackend::wantsKeyboard() const
{
	return ImGui::GetIO().WantCaptureKeyboard;
}


void ImGuiBackend::draw(ITexture& target)
{
	ImGui::Render();
	const ImDrawData& data = *ImGui::GetDrawData();
	if (data.TotalVtxCount == 0 || data.TotalIdxCount == 0 || data.DisplaySize.x <= 0.0f || data.DisplaySize.y <= 0.0f)
	{
		return;
	}

	FrameBuffers& frame = frames_[frame_index_];
	if (frame.fence != 0)
	{
		render_device_.waitForDirectQueue(frame.fence);
	}
	std::vector<ImDrawVert> vertices;
	std::vector<uint32_t> indices;
	vertices.reserve(data.TotalVtxCount);
	indices.reserve(data.TotalIdxCount);
	for (const ImDrawList* list : data.CmdLists)
	{
		vertices.insert(vertices.end(), list->VtxBuffer.begin(), list->VtxBuffer.end());
		indices.insert(indices.end(), list->IdxBuffer.begin(), list->IdxBuffer.end());
	}
	const size_t vertex_size = vertices.size() * sizeof(ImDrawVert);
	const size_t index_size = indices.size() * sizeof(uint32_t);
	if (!ensureBuffer(frame.vertices, vertex_size, false) || !ensureBuffer(frame.indices, index_size, true))
	{
		return;
	}
	frame.vertices->upload(vertices.data(), vertex_size);
	frame.indices->upload(indices.data(), index_size);

	const float width = static_cast<float>(target.getWidth());
	const float height = static_cast<float>(target.getHeight());
	const float scale_x = 2.0f / data.DisplaySize.x;
	const float scale_y = -2.0f / data.DisplaySize.y;
	const float transform[] = { scale_x, scale_y, -1.0f - data.DisplayPos.x * scale_x, 1.0f - data.DisplayPos.y * scale_y };
	ICommandList& commands = render_device_.acquireCommandList(ECommandListType::kDirect, "ImGuiPass");
	commands.transitionTexture(target, EResourceState::kPresent, EResourceState::kRenderTarget);
	commands.setRenderTarget(target);
	const auto set_draw_state = [&]()
	{
		commands.setViewport(0.0f, 0.0f, width, height);
		commands.setPipelineState(*pipeline_state_);
		commands.setPrimitiveTopology(EPrimitiveTopology::kTriangle);
		commands.setGraphicsRoot32BitConstants(0, 4, transform);
		commands.setVertexBuffer(*frame.vertices, sizeof(ImDrawVert));
		commands.setIndexBuffer(*frame.indices);
	};
	set_draw_state();

	const ITexture* bound_texture = nullptr;
	uint32_t index_offset = 0;
	int32_t vertex_offset = 0;
	for (const ImDrawList* list : data.CmdLists)
	{
		for (const ImDrawCmd& draw : list->CmdBuffer)
		{
			if (draw.UserCallback)
			{
				if (draw.UserCallback == ImDrawCallback_ResetRenderState)
				{
					set_draw_state();
					bound_texture = nullptr;
				}
				else
				{
					draw.UserCallback(list, &draw);
				}
				continue;
			}

			const ImVec4& clip = draw.ClipRect;
			const float left = std::clamp((clip.x - data.DisplayPos.x) * data.FramebufferScale.x, 0.0f, width);
			const float top = std::clamp((clip.y - data.DisplayPos.y) * data.FramebufferScale.y, 0.0f, height);
			const float right = std::clamp((clip.z - data.DisplayPos.x) * data.FramebufferScale.x, 0.0f, width);
			const float bottom = std::clamp((clip.w - data.DisplayPos.y) * data.FramebufferScale.y, 0.0f, height);
			if (right <= left || bottom <= top)
			{
				continue;
			}

			const ITexture* texture = draw.GetTexID() ? reinterpret_cast<const ITexture*>(draw.GetTexID()) : font_texture_;
			if (texture != bound_texture)
			{
				commands.setDescriptorSet(1, { DescriptorResource::textureSRV(texture) });
				bound_texture = texture;
			}
			commands.setScissorRect(static_cast<int32_t>(left), static_cast<int32_t>(top),
				static_cast<int32_t>(right), static_cast<int32_t>(bottom));
			commands.drawIndexed(draw.ElemCount, index_offset + draw.IdxOffset, vertex_offset + static_cast<int32_t>(draw.VtxOffset));
		}
		index_offset += static_cast<uint32_t>(list->IdxBuffer.Size);
		vertex_offset += list->VtxBuffer.Size;
	}

	commands.transitionTexture(target, EResourceState::kRenderTarget, EResourceState::kPresent);
	render_device_.submitCommandList(commands);
	frame.fence = render_device_.executeCommandLists(ECommandListType::kDirect);
	frame_index_ = (frame_index_ + 1) % frames_.size();
}


ImGuiBackend::~ImGuiBackend()
{
	GpuResourceManager& resources = render_device_.getResourceManager();
	for (const FrameBuffers& frame : frames_)
	{
		if (frame.fence != 0)
		{
			render_device_.waitForDirectQueue(frame.fence);
		}
		if (frame.vertices)
		{
			resources.releaseBuffer(*frame.vertices);
		}
		if (frame.indices)
		{
			resources.releaseBuffer(*frame.indices);
		}
	}
	if (font_texture_)
	{
		resources.releaseTexture(*font_texture_);
	}
	ImGui::DestroyContext(context_);
}


ImGuiBackend::ImGuiBackend(IRenderDevice& render_device)
	: render_device_(render_device)
{}


bool ImGuiBackend::init()
{
	IMGUI_CHECKVERSION();
	context_ = ImGui::CreateContext();
	ASSERT_MSG_RETURN(context_ != nullptr, false, "Failed to create ImGui context");
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.BackendRendererName = "BorealEngine";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();

	unsigned char* pixels = nullptr;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	ITexture::Desc texture_desc = {};
	texture_desc.width = static_cast<uint32_t>(width);
	texture_desc.height = static_cast<uint32_t>(height);
	texture_desc.format = ETextureFormat::kRGBA8_Unorm;
	font_texture_ = render_device_.getResourceManager().createTexture(texture_desc);
	ASSERT_MSG_RETURN(font_texture_ != nullptr, false, "Failed to create ImGui font texture");
	font_texture_->uploadPixels(pixels, texture_desc.width * texture_desc.height * 4);
	SET_RENDER_OBJECT_DEBUG_NAME(*font_texture_, "ImGuiFontAtlas");
	io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(font_texture_));

	static_assert(offsetof(ImDrawVert, pos) == 0 && offsetof(ImDrawVert, uv) == 8 &&
		offsetof(ImDrawVert, col) == 16 && sizeof(ImDrawVert) == 20);
	GraphicsPipelineDesc desc = {};
	desc.shader_state.shader_name = "imgui";
	desc.input_layout_state.addAttribute(ESemanticType::kPosition, ETextureFormat::kRG32_Float);
	desc.input_layout_state.addAttribute(ESemanticType::kTexCoord, ETextureFormat::kRG32_Float);
	desc.input_layout_state.addAttribute(ESemanticType::kColor, ETextureFormat::kRGBA8_Unorm);
	desc.resource_layout_state[0] = RootParameter::makeConstants32Bits(0, 4);
	desc.resource_layout_state[1] = RootParameter::makeDescriptorTable({ DescriptorRange{ DescriptorType::kSRV, 0, 1 } });
	desc.output_merger_state.addRenderTarget(ETextureFormat::kRGBA8_Unorm, EBlendMode::kAlpha);
	desc.output_merger_state.depth_stencil.depth_enable = false;
	desc.output_merger_state.depth_stencil.depth_write = false;
	pipeline_state_ = &render_device_.getPipelineStateManager().createPipelineState(desc);
	return true;
}


bool ImGuiBackend::ensureBuffer(IBuffer*& buffer, size_t required_size, bool index_buffer)
{
	if (buffer && buffer->getDesc().size >= required_size)
	{
		return true;
	}
	IBuffer::Desc desc = {};
	desc.size = required_size + required_size / 2;
	desc.usage = index_buffer ? EBufferUsage::kIndex : EBufferUsage::kVertex;
	GpuResourceManager& resources = render_device_.getResourceManager();
	IBuffer* replacement = resources.createBuffer(desc);
	if (!replacement)
	{
		return false;
	}
	if (buffer)
	{
		resources.releaseBuffer(*buffer);
	}
	buffer = replacement;
	return true;
}

}  // namespace render
