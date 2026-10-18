#pragma once

#include <engine/core/utility/utility.h>
#include <engine/platform/window.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace platform {

// Keyboard and mouse keys
enum class EKey : uint32_t
{
    kUnknown = 0,

    // Letters
    kA, kB, kC, kD, kE, kF, kG, kH, kI, kJ, kK, kL, kM,
    kN, kO, kP, kQ, kR, kS, kT, kU, kV, kW, kX, kY, kZ,

    // Digits
    k0, k1, k2, k3, k4, k5, k6, k7, k8, k9,

    // Function keys
    kF1, kF2, kF3, kF4, kF5, kF6, kF7, kF8, kF9, kF10, kF11, kF12,

    // Navigation
    kUp, kDown, kLeft, kRight,
    kHome, kEnd, kPageUp, kPageDown,
    kInsert, kDelete,

    // Control
    kEscape, kReturn, kSpace, kBackspace, kTab,
    kLeftShift, kRightShift,
    kLeftCtrl,  kRightCtrl,
    kLeftAlt,   kRightAlt,

    // Mouse
    kMouseLeft, kMouseRight, kMouseMiddle,

    kCount
};

// Named action mapping
struct ActionBinding
{
    std::string name;     // action name, e.g. "move_forward"
    EKey        key;
};

struct InputConfig
{
    std::vector<ActionBinding> bindings;
};

struct FpsCameraSettings
{
    float move_speed        = 5.0f;
    float sprint_multiplier = 3.0f;
    float mouse_sensitivity = 0.15f;
};

// Per-frame input state
class IInput : public utility::NonCopyable
{
public:
	// Load action bindings from a JSON file and register Raw Input for the given window.
	// The file path is: <data_root_path>/<config_name>.json
	// window_handle is the platform window handle (HWND on Windows).
	static std::unique_ptr<IInput> create(const std::string& data_root_path,
	                                      const std::string& config_name,
	                                      WindowHandle window_handle);

	virtual ~IInput() = default;

	// Called every frame to capture current key states.
	// Must be called after IWindow::pumpEvents() so WM_INPUT messages are processed.
	virtual void update() = 0;

	// Low-level key queries
	virtual bool isKeyDown(EKey key) const = 0;
	virtual bool isKeyPressed(EKey key) const = 0;   // true only on first frame
	virtual bool isKeyReleased(EKey key) const = 0;  // true only on release frame

	// Mouse delta in pixels since last update (only accumulated while RMB is held)
	virtual void getMouseDelta(float& out_dx, float& out_dy) const = 0;

	// High-level action queries (use action name from config)
	virtual bool isActionDown(const std::string& action) const = 0;
	virtual bool isActionPressed(const std::string& action) const = 0;
	virtual bool isActionReleased(const std::string& action) const = 0;

	virtual const FpsCameraSettings& getFpsCameraSettings() const = 0;
};

// Called by the platform window's WindowProc on WM_INPUT to forward raw mouse deltas
void notifyRawMouseInput(long dx, long dy);

}  // namespace platform
