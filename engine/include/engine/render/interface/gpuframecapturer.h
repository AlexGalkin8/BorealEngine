#pragma once

#include <memory>
#include <string>


namespace render {

enum class ECaptureTool
{
	kNone,
	kRenderDoc,
};


enum class ECaptureToolOpenMode
{
	kNone,
	kLive,
	kCapture,
};


struct FrameCaptureSettings
{
	ECaptureTool tool = ECaptureTool::kNone;
	std::wstring tool_path;
	ECaptureToolOpenMode open_mode = ECaptureToolOpenMode::kNone;
};


class IGpuFrameCapturer
{
public:
	// Load the capture tool before creating a graphics device.
	static std::unique_ptr<IGpuFrameCapturer> create(const FrameCaptureSettings& settings);

	virtual ~IGpuFrameCapturer() = default;

	void requestFrameCapture();
	void beginFrameCapture();
	void endFrameCapture();

private:
	virtual void startCapture() = 0;
	virtual void finishCapture() = 0;

	enum class EFrameCaptureState
	{
		kIdle,
		kRequested,
		kCapturing,
	};

	EFrameCaptureState frame_capture_state_ = EFrameCaptureState::kIdle;
};

}  // namespace render
