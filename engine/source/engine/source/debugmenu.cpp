#include "debugmenu.h"

#include <engine/render/interface/renderdevice.h>
#include <engine/render/renderer.h>
#include <engine/scene/scene.h>

#include <imgui.h>

#include <utility>


namespace engine {

static void drawPointLights(scene::Scene& scene, bool& is_open)
{
	if (ImGui::Begin("Point lights", &is_open, ImGuiWindowFlags_AlwaysAutoResize))
	{
		std::vector<scene::PointLight>& lights = scene.getPointLights();
		for (size_t index = 0; index < lights.size(); ++index)
		{
			ImGui::PushID(static_cast<int>(index));
			ImGui::SeparatorText(("Point light " + std::to_string(index + 1)).c_str());
			scene::PointLight& light = lights[index];
			ImGui::Checkbox("Enabled", &light.enabled);
			float position[] = { light.position.x, light.position.y, light.position.z };
			if (ImGui::DragFloat3("Position", position, 0.05f))
			{
				light.position = { position[0], position[1], position[2] };
			}
			float color[] = { light.color.x, light.color.y, light.color.z };
			if (ImGui::ColorEdit3("Color", color, ImGuiColorEditFlags_Float))
			{
				light.color = { color[0], color[1], color[2] };
			}
			ImGui::DragFloat("Intensity", &light.intensity, 0.05f, 0.0f, 1000.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragFloat("Radius", &light.radius, 0.05f, 0.01f, 1000.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			const bool remove = ImGui::Button("Remove");
			ImGui::PopID();
			if (remove)
			{
				lights.erase(lights.begin() + index);
				break;
			}
		}

		ImGui::BeginDisabled(lights.size() >= scene::kMaxPointLights);
		if (ImGui::Button("Add point light"))
		{
			lights.emplace_back();
		}
		ImGui::EndDisabled();
	}
	ImGui::End();
}


DebugMenu::DebugMenu(render::IRenderDevice& render_device, std::vector<std::string> scene_names,
	const std::string& active_scene_name)
	: render_device_(render_device)
	, scene_names_(std::move(scene_names))
	, active_scene_name_(active_scene_name)
{}


std::string DebugMenu::draw(scene::Scene& scene, render::IRenderer& renderer)
{
	const render::RenderSettings& render_settings = renderer.getRenderSettings();
	std::string selected_scene;
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
		if (ImGui::BeginMenu("Scene", !scene_names_.empty()))
		{
			for (const std::string& scene_name : scene_names_)
			{
				const bool active = scene_name == active_scene_name_;
				if (ImGui::MenuItem(scene_name.c_str(), nullptr, active) && !active)
				{
					selected_scene = scene_name;
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Render"))
		{
			ImGui::MenuItem("Directional light", nullptr, &show_directional_light_);
			ImGui::MenuItem("Point lights", nullptr, &show_point_lights_);
			ImGui::MenuItem("Rendering statistics", nullptr, &show_rendering_stats_);
			if (ImGui::BeginMenu("Lighting model"))
			{
				using render::ELightingModel;
				struct LightingModel
				{
					const char* label;
					ELightingModel model;
				};
				static constexpr LightingModel kLightingModels[] = {
					{ "Diffuse", ELightingModel::kDiffuse },
					{ "Blinn-Phong", ELightingModel::kBlinnPhong },
					{ "PBR", ELightingModel::kPBR },
				};
				for (const LightingModel& lighting : kLightingModels)
				{
					if (ImGui::MenuItem(lighting.label, nullptr, render_settings.lighting_model == lighting.model))
					{
						renderer.setLightingModel(lighting.model);
					}
				}
				ImGui::EndMenu();
			}
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
					{ "Roughness", EDebugRenderView::kRoughness },
					{ "Metallic", EDebugRenderView::kMetallic },
				};
				for (const View& view : kViews)
				{
					if (ImGui::MenuItem(view.label, nullptr, render_settings.debug_view == view.mode))
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
			if (ImGui::MenuItem("Pause when unfocused", nullptr, pause_on_focus_lost_))
			{
				pause_on_focus_lost_ = !pause_on_focus_lost_;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools"))
		{
			ImGui::MenuItem("ImGui Metrics", nullptr, &show_imgui_metrics_);
			if (ImGui::MenuItem("Capture frame", nullptr, false, render_device_.hasFrameCapturer()))
			{
				render_device_.requestFrameCapture();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();

	if (!visible_)
	{
		return selected_scene;
	}

	if (show_directional_light_)
	{
		if (ImGui::Begin("Directional light", &show_directional_light_, ImGuiWindowFlags_AlwaysAutoResize))
		{
			scene::DirectionalLight& light = scene.getDirectionalLight();
			ImGui::Checkbox("Enabled", &light.enabled);
			float direction[] = { light.direction_to_light.x, light.direction_to_light.y, light.direction_to_light.z };
			if (ImGui::SliderFloat3("Direction to light", direction, -1.0f, 1.0f))
			{
				light.direction_to_light = { direction[0], direction[1], direction[2] };
			}

			float color[] = { light.color.x, light.color.y, light.color.z };
			if (ImGui::ColorEdit3("Color", color, ImGuiColorEditFlags_Float))
			{
				light.color = { color[0], color[1], color[2] };
			}
			ImGui::DragFloat("Intensity", &light.intensity, 0.05f, 0.0f, 0.0f);
			if (light.intensity < 0.0f)
			{
				light.intensity = 0.0f;
			}
		}
		ImGui::End();
	}
	if (show_point_lights_)
	{
		drawPointLights(scene, show_point_lights_);
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
			ImGui::Text("Back buffer: %u x %u", render_settings.render_width, render_settings.render_height);
		}
		ImGui::End();
	}
	if (show_imgui_metrics_)
	{
		ImGui::ShowMetricsWindow(&show_imgui_metrics_);
	}
	return selected_scene;
}

}  // namespace engine
