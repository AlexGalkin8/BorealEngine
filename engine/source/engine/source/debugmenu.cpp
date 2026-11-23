#include "debugmenu.h"

#include <engine/render/interface/renderdevice.h>
#include <engine/render/renderer.h>

#include <imgui.h>


namespace engine {

void DebugMenu::draw(uint32_t width, uint32_t height, bool& pause_on_focus_lost, render::IRenderDevice& render_device,
	render::IRenderer& renderer)
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9.0f, 5.0f));
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.08f, 0.12f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.13f, 0.34f, 0.35f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.17f, 0.42f, 0.43f, 1.0f));
	float menu_height = ImGui::GetFrameHeight();
	if (ImGui::BeginMainMenuBar())
	{
		menu_height = ImGui::GetWindowHeight();
		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Hide debug interface", "F1"))
			{
				visible_ = false;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Render"))
		{
			ImGui::MenuItem("Rendering statistics", nullptr, &show_rendering_stats_);
			if (ImGui::BeginMenu("Debug view"))
			{
				using render::EDebugRenderView;
				struct View
				{
					const char* label;
					EDebugRenderView mode;
				};
				static constexpr View kViews[] = {
					{ "Final", EDebugRenderView::kFinal },
					{ "G-buffer albedo", EDebugRenderView::kAlbedo },
					{ "G-buffer world normals", EDebugRenderView::kWorldNormal },
					{ "HDR lighting (clipped SDR)", EDebugRenderView::kLightingHDR },
				};
				for (const View& view : kViews)
				{
					if (ImGui::MenuItem(view.label, nullptr, renderer.getRenderSettings().debug_view == view.mode))
					{
						renderer.setDebugView(view.mode);
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Engine"))
		{
			if (ImGui::MenuItem("Pause when unfocused", nullptr, pause_on_focus_lost))
			{
				pause_on_focus_lost = !pause_on_focus_lost;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools"))
		{
			ImGui::MenuItem("ImGui Metrics", nullptr, &show_imgui_metrics_);
			if (ImGui::MenuItem("Capture frame", nullptr, false, render_device.hasFrameCapturer()))
			{
				render_device.requestFrameCapture();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();

	if (!visible_)
	{
		return;
	}

	if (show_rendering_stats_)
	{
		ImGui::SetNextWindowPos(ImVec2(12.0f, menu_height + 8.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(270.0f, 125.0f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Rendering statistics", &show_rendering_stats_))
		{
			const ImGuiIO& io = ImGui::GetIO();
			ImGui::Text("Frame time: %.2f ms", io.DeltaTime * 1000.0f);
			ImGui::Text("Frame rate: %.1f FPS", io.Framerate);
			ImGui::Separator();
			ImGui::Text("Back buffer: %u x %u", width, height);
		}
		ImGui::End();
	}
	if (show_imgui_metrics_)
	{
		ImGui::ShowMetricsWindow(&show_imgui_metrics_);
	}
}

}  // namespace engine
