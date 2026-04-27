#pragma once

#include <engine/core/utility/utility.h>

#include <cstdint>
#include <memory>
#include <string>


namespace platform {

using WindowHandle = void*;


class IWindow : public utility::NonCopyable
{
public:
	enum class EMode
	{
		kWindow = 0,
		kBorderless,
		kFullscreen
	};

	struct Desc
	{
		std::wstring name = L"BorealEngine";
		EMode mode = EMode::kWindow;
		uint32_t width = 1280;
		uint32_t height = 720;
	};

public:
	static std::unique_ptr<IWindow> create(const Desc& desc);

	IWindow(const Desc& desc, WindowHandle handle);
	~IWindow();

	// Returns false when quit message was received.
	bool pumpEvents();

	// True when window is foreground and not minimized.
	bool isActive() const;

	bool consumeResize(uint32_t& out_width, uint32_t& out_height);

	WindowHandle getHandle() const;

	uint32_t getWidth() const;
	uint32_t getHeight() const;

	void resize(uint32_t new_width, uint32_t new_height);

private:
	Desc desc_;
	WindowHandle handle_;
	bool has_pending_resize_ = false;
};

}  // namespace platform
