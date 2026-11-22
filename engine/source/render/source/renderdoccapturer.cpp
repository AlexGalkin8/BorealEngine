#include "interface/gpuframecapturer.h"

#include <engine/core/utility/assert.h>
#include <engine/core/utility/log.h>

#include <filesystem>
#include <format>

#include <windows.h>
#include <renderdoc_app.h>


namespace render {

namespace {

void openRenderDocUi(RENDERDOC_API_1_4_1& api, bool connect_live, const char* arguments)
{
	const uint32_t ui_pid = api.LaunchReplayUI(connect_live ? 1u : 0u, arguments);
	if (ui_pid)
	{
		LOG_INFO("RenderDoc", "Opened RenderDoc UI (PID {}).", ui_pid);
	}
	else
	{
		LOG_WARNING("RenderDoc", "Could not open the RenderDoc UI. Frame capture is still available.");
	}
}


class RenderDocCapturer final : public IGpuFrameCapturer
{
public:
	RenderDocCapturer(RENDERDOC_API_1_4_1& api, ECaptureToolOpenMode open_mode)
		: api_(api), open_mode_(open_mode) {}

private:
	void startCapture() override
	{
		api_.StartFrameCapture(nullptr, nullptr);
	}

	void finishCapture() override
	{
		if (!api_.EndFrameCapture(nullptr, nullptr))
		{
			LOG_WARNING("RenderDoc", "Frame capture could not be saved.");
			return;
		}

		const uint32_t capture_count = api_.GetNumCaptures();
		uint32_t path_length = 0;
		if (capture_count == 0 || !api_.GetCapture(capture_count - 1, nullptr, &path_length, nullptr))
		{
			LOG_INFO("RenderDoc", "Frame capture saved.");
			return;
		}

		std::string path(path_length, '\0');
		if (api_.GetCapture(capture_count - 1, path.data(), &path_length, nullptr))
		{
			LOG_INFO("RenderDoc", "Frame capture saved to {}.", path.c_str());
			if (open_mode_ == ECaptureToolOpenMode::kCapture)
			{
				const std::string arguments = std::format("\"{}\"", path.c_str());
				openRenderDocUi(api_, false, arguments.c_str());
			}
		}
	}

	RENDERDOC_API_1_4_1& api_;
	ECaptureToolOpenMode open_mode_;
};


std::filesystem::path getDefaultRenderDocPath()
{
	const DWORD size = GetEnvironmentVariableW(L"ProgramFiles", nullptr, 0);
	if (size == 0)
	{
		return {};
	}

	std::wstring program_files(size, L'\0');
	const DWORD length = GetEnvironmentVariableW(L"ProgramFiles", program_files.data(), size);
	if (length == 0 || length >= size)
	{
		return {};
	}
	program_files.resize(length);
	return std::filesystem::path(program_files) / L"RenderDoc" / L"renderdoc.dll";
}

}  // namespace


std::unique_ptr<IGpuFrameCapturer> IGpuFrameCapturer::create(const FrameCaptureSettings& settings)
{
	if (settings.tool != ECaptureTool::kRenderDoc)
	{
		return nullptr;
	}

	// Keep installed hooks and their module alive until process exit.
	HMODULE module = GetModuleHandleW(L"renderdoc.dll");
	if (!module)
	{
		std::error_code error;
		const std::filesystem::path dll_path = settings.tool_path.empty()
			? getDefaultRenderDocPath() : std::filesystem::absolute(settings.tool_path, error);
		if (error || dll_path.empty())
		{
			LOG_WARNING("RenderDoc", "Could not resolve the RenderDoc DLL path. Starting normally.");
			return nullptr;
		}
		module = LoadLibraryExW(dll_path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		if (!module)
		{
			const DWORD load_error = GetLastError();
			LOG_WARNING("RenderDoc", "Could not load renderdoc.dll (Win32 error {}). Starting normally.", load_error);
			return nullptr;
		}
	}

	const pRENDERDOC_GetAPI get_api = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(module, "RENDERDOC_GetAPI"));
	void* api = nullptr;
	if (!get_api || !get_api(eRENDERDOC_API_Version_1_4_1, &api) || !api)
	{
		LOG_WARNING("RenderDoc", "Could not initialize the RenderDoc capture API. Starting normally.");
		return nullptr;
	}

	LOG_INFO("RenderDoc", "RenderDoc capture API initialized.");
	RENDERDOC_API_1_4_1& renderdoc_api = *static_cast<RENDERDOC_API_1_4_1*>(api);
	if (settings.open_mode == ECaptureToolOpenMode::kLive && !renderdoc_api.IsTargetControlConnected())
	{
		openRenderDocUi(renderdoc_api, true, nullptr);
	}
	std::unique_ptr<IGpuFrameCapturer> capturer = std::make_unique<RenderDocCapturer>(renderdoc_api, settings.open_mode);
	if (settings.capture_first_frame || settings.open_mode == ECaptureToolOpenMode::kCapture)
	{
		capturer->requestFrameCapture();
	}
	return capturer;
}


void IGpuFrameCapturer::requestFrameCapture()
{
	ASSERT_MSG_RETURN_VOID(frame_capture_state_ != EFrameCaptureState::kCapturing, "A GPU frame capture is already active");
	frame_capture_state_ = EFrameCaptureState::kRequested;
}


void IGpuFrameCapturer::beginFrameCapture()
{
	if (frame_capture_state_ == EFrameCaptureState::kRequested)
	{
		frame_capture_state_ = EFrameCaptureState::kCapturing;
		startCapture();
	}
}


void IGpuFrameCapturer::endFrameCapture()
{
	if (frame_capture_state_ == EFrameCaptureState::kCapturing)
	{
		finishCapture();
		frame_capture_state_ = EFrameCaptureState::kIdle;
	}
}

}  // namespace render
