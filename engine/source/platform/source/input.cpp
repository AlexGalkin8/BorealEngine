#include <engine/platform/input.h>

#include <engine/core/utility/assert.h>

#include <windows.h>

#include <array>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>


namespace platform {

class Win32Input;

// Global pointer to active input instance for use from WindowProc
static Win32Input* g_active_input = nullptr;

// ---- JSON helpers (no external deps) --------------------------------------

static std::string readTextFile(const std::string& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file)
    {
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Returns all string values for a repeated key inside a JSON array of objects.
// Looks for patterns like: "key" : "value"
static std::vector<std::string> extractArrayStringValues(std::string_view text, std::string_view key)
{
    std::vector<std::string> result;
    const std::string pattern = std::string("\"") + std::string(key) + "\"";
    size_t search_pos = 0;
    while (true)
    {
        const size_t key_pos = text.find(pattern, search_pos);
        if (key_pos == std::string_view::npos)
        {
            break;
        }
        search_pos = key_pos + pattern.size();

        const size_t colon_pos = text.find(':', search_pos);
        if (colon_pos == std::string_view::npos)
        {
            break;
        }

        const size_t first_quote = text.find('"', colon_pos + 1);
        if (first_quote == std::string_view::npos)
        {
            break;
        }

        const size_t second_quote = text.find('"', first_quote + 1);
        if (second_quote == std::string_view::npos)
        {
            break;
        }

        result.emplace_back(text.substr(first_quote + 1, second_quote - first_quote - 1));
        search_pos = second_quote + 1;
    }
    return result;
}

static bool extractNumberValue(std::string_view text, std::string_view key, float& out_value)
{
    const std::string pattern = std::string("\"") + std::string(key) + "\"";
    const size_t key_pos = text.find(pattern);
    if (key_pos == std::string_view::npos)
    {
        return false;
    }

    const size_t colon_pos = text.find(':', key_pos + pattern.size());
    if (colon_pos == std::string_view::npos)
    {
        return false;
    }

    size_t num_begin = colon_pos + 1;
    while (num_begin < text.size() && (text[num_begin] == ' ' || text[num_begin] == '\t' || text[num_begin] == '\r' || text[num_begin] == '\n'))
    {
        ++num_begin;
    }

    size_t num_end = num_begin;
    while (num_end < text.size())
    {
        const char c = text[num_end];
        if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E')
        {
            ++num_end;
        }
        else
        {
            break;
        }
    }

    if (num_end <= num_begin)
    {
        return false;
    }

    try
    {
        out_value = std::stof(std::string(text.substr(num_begin, num_end - num_begin)));
    }
    catch (...)
    {
        return false;
    }

    return true;
}

// ---- Key name -> EKey mapping ---------------------------------------------

static const std::unordered_map<std::string, EKey>& keyNameMap()
{
    static const std::unordered_map<std::string, EKey> map = {
        { "A", EKey::kA }, { "B", EKey::kB }, { "C", EKey::kC }, { "D", EKey::kD },
        { "E", EKey::kE }, { "F", EKey::kF }, { "G", EKey::kG }, { "H", EKey::kH },
        { "I", EKey::kI }, { "J", EKey::kJ }, { "K", EKey::kK }, { "L", EKey::kL },
        { "M", EKey::kM }, { "N", EKey::kN }, { "O", EKey::kO }, { "P", EKey::kP },
        { "Q", EKey::kQ }, { "R", EKey::kR }, { "S", EKey::kS }, { "T", EKey::kT },
        { "U", EKey::kU }, { "V", EKey::kV }, { "W", EKey::kW }, { "X", EKey::kX },
        { "Y", EKey::kY }, { "Z", EKey::kZ },
        { "0", EKey::k0 }, { "1", EKey::k1 }, { "2", EKey::k2 }, { "3", EKey::k3 },
        { "4", EKey::k4 }, { "5", EKey::k5 }, { "6", EKey::k6 }, { "7", EKey::k7 },
        { "8", EKey::k8 }, { "9", EKey::k9 },
        { "F1",  EKey::kF1  }, { "F2",  EKey::kF2  }, { "F3",  EKey::kF3  },
        { "F4",  EKey::kF4  }, { "F5",  EKey::kF5  }, { "F6",  EKey::kF6  },
        { "F7",  EKey::kF7  }, { "F8",  EKey::kF8  }, { "F9",  EKey::kF9  },
        { "F10", EKey::kF10 }, { "F11", EKey::kF11 }, { "F12", EKey::kF12 },
        { "Up",       EKey::kUp       }, { "Down",  EKey::kDown  },
        { "Left",     EKey::kLeft     }, { "Right", EKey::kRight },
        { "Home",     EKey::kHome     }, { "End",   EKey::kEnd   },
        { "PageUp",   EKey::kPageUp   }, { "PageDown", EKey::kPageDown },
        { "Insert",   EKey::kInsert   }, { "Delete",   EKey::kDelete  },
        { "Escape",   EKey::kEscape   }, { "Return",   EKey::kReturn  },
        { "Space",    EKey::kSpace    }, { "Backspace", EKey::kBackspace },
        { "Tab",      EKey::kTab      },
        { "LeftShift",  EKey::kLeftShift  }, { "RightShift",  EKey::kRightShift  },
        { "LeftCtrl",   EKey::kLeftCtrl   }, { "RightCtrl",   EKey::kRightCtrl   },
        { "LeftAlt",    EKey::kLeftAlt    }, { "RightAlt",    EKey::kRightAlt    },
        { "MouseLeft",   EKey::kMouseLeft   },
        { "MouseRight",  EKey::kMouseRight  },
        { "MouseMiddle", EKey::kMouseMiddle },
    };
    return map;
}

static EKey keyFromName(const std::string& name)
{
    const auto& map = keyNameMap();
    const auto it = map.find(name);
    return (it != map.end()) ? it->second : EKey::kUnknown;
}

// ---- EKey -> Virtual Key--------------------------------------------------

static int virtualKeyFromEKey(EKey key)
{
    switch (key)
    {
    case EKey::kA: return 'A'; case EKey::kB: return 'B'; case EKey::kC: return 'C';
    case EKey::kD: return 'D'; case EKey::kE: return 'E'; case EKey::kF: return 'F';
    case EKey::kG: return 'G'; case EKey::kH: return 'H'; case EKey::kI: return 'I';
    case EKey::kJ: return 'J'; case EKey::kK: return 'K'; case EKey::kL: return 'L';
    case EKey::kM: return 'M'; case EKey::kN: return 'N'; case EKey::kO: return 'O';
    case EKey::kP: return 'P'; case EKey::kQ: return 'Q'; case EKey::kR: return 'R';
    case EKey::kS: return 'S'; case EKey::kT: return 'T'; case EKey::kU: return 'U';
    case EKey::kV: return 'V'; case EKey::kW: return 'W'; case EKey::kX: return 'X';
    case EKey::kY: return 'Y'; case EKey::kZ: return 'Z';
    case EKey::k0: return '0'; case EKey::k1: return '1'; case EKey::k2: return '2';
    case EKey::k3: return '3'; case EKey::k4: return '4'; case EKey::k5: return '5';
    case EKey::k6: return '6'; case EKey::k7: return '7'; case EKey::k8: return '8';
    case EKey::k9: return '9';
    case EKey::kF1:  return VK_F1;  case EKey::kF2:  return VK_F2;
    case EKey::kF3:  return VK_F3;  case EKey::kF4:  return VK_F4;
    case EKey::kF5:  return VK_F5;  case EKey::kF6:  return VK_F6;
    case EKey::kF7:  return VK_F7;  case EKey::kF8:  return VK_F8;
    case EKey::kF9:  return VK_F9;  case EKey::kF10: return VK_F10;
    case EKey::kF11: return VK_F11; case EKey::kF12: return VK_F12;
    case EKey::kUp:       return VK_UP;
    case EKey::kDown:     return VK_DOWN;
    case EKey::kLeft:     return VK_LEFT;
    case EKey::kRight:    return VK_RIGHT;
    case EKey::kHome:     return VK_HOME;
    case EKey::kEnd:      return VK_END;
    case EKey::kPageUp:   return VK_PRIOR;
    case EKey::kPageDown: return VK_NEXT;
    case EKey::kInsert:   return VK_INSERT;
    case EKey::kDelete:   return VK_DELETE;
    case EKey::kEscape:   return VK_ESCAPE;
    case EKey::kReturn:   return VK_RETURN;
    case EKey::kSpace:    return VK_SPACE;
    case EKey::kBackspace: return VK_BACK;
    case EKey::kTab:      return VK_TAB;
    case EKey::kLeftShift:   return VK_LSHIFT;
    case EKey::kRightShift:  return VK_RSHIFT;
    case EKey::kLeftCtrl:    return VK_LCONTROL;
    case EKey::kRightCtrl:   return VK_RCONTROL;
    case EKey::kLeftAlt:     return VK_LMENU;
    case EKey::kRightAlt:    return VK_RMENU;
    case EKey::kMouseLeft:   return VK_LBUTTON;
    case EKey::kMouseRight:  return VK_RBUTTON;
    case EKey::kMouseMiddle: return VK_MBUTTON;
    default: return 0;
    }
}

// ---- Win32 IInput implementation -----------------------------------------

class Win32Input final : public IInput
{
public:
    Win32Input() = default;
    ~Win32Input() override;

    bool init(const std::string& data_root_path, const std::string& config_name, HWND hwnd);

    void update() override;

    bool isKeyDown(EKey key) const override;
    bool isKeyPressed(EKey key) const override;
    bool isKeyReleased(EKey key) const override;

    void getMouseDelta(float& out_dx, float& out_dy) const override;

    bool isActionDown(const std::string& action) const override;
    bool isActionPressed(const std::string& action) const override;
    bool isActionReleased(const std::string& action) const override;

    const FpsCameraSettings& getFpsCameraSettings() const override { return fps_camera_settings_; }

    void accumulateRawDelta(long dx, long dy) { raw_dx_ += dx; raw_dy_ += dy; }

private:
    static constexpr uint32_t kKeyCount = static_cast<uint32_t>(EKey::kCount);

    std::array<bool, kKeyCount> current_  = {};
    std::array<bool, kKeyCount> previous_ = {};

    // Raw mouse delta accumulated between update() calls
    long raw_dx_ = 0;
    long raw_dy_ = 0;

    float mouse_dx_ = 0.0f;
    float mouse_dy_ = 0.0f;

    bool rmb_was_down_ = false;

    HWND hwnd_ = nullptr;
    std::unordered_map<std::string, EKey> action_map_;
    FpsCameraSettings fps_camera_settings_;
};

static void accumulateRawMouseDelta(long dx, long dy)
{
    if (g_active_input)
    {
        g_active_input->accumulateRawDelta(dx, dy);
    }
}

void notifyRawMouseInput(long dx, long dy)
{
    accumulateRawMouseDelta(dx, dy);
}


Win32Input::~Win32Input()
{
    if (hwnd_)
    {
        ShowCursor(TRUE);
    }
    if (g_active_input == this)
    {
        g_active_input = nullptr;
    }
}


bool Win32Input::init(const std::string& data_root_path, const std::string& config_name, HWND hwnd)
{
    hwnd_ = hwnd;
    g_active_input = this;

    // Register Raw Input for mouse
    RAWINPUTDEVICE rid{};
    rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
    rid.usUsage     = 0x02; // HID_USAGE_GENERIC_MOUSE
    rid.dwFlags     = RIDEV_INPUTSINK; // receive input even when not foreground
    rid.hwndTarget  = hwnd_;
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        return false;
    }

    // Load bindings
    const std::string path = data_root_path + "/" + config_name + ".json";
    const std::string text = readTextFile(path);
    if (text.empty())
    {
        return true;
    }

    const std::vector<std::string> actions = extractArrayStringValues(text, "action");
    const std::vector<std::string> keys    = extractArrayStringValues(text, "key");

    const size_t count = std::min(actions.size(), keys.size());
    for (size_t i = 0; i < count; ++i)
    {
        const EKey key = keyFromName(keys[i]);
        if (key != EKey::kUnknown)
        {
            action_map_[actions[i]] = key;
        }
    }

    extractNumberValue(text, "move_speed",        fps_camera_settings_.move_speed);
    extractNumberValue(text, "sprint_multiplier", fps_camera_settings_.sprint_multiplier);
    extractNumberValue(text, "mouse_sensitivity",  fps_camera_settings_.mouse_sensitivity);

    return true;
}


void Win32Input::update()
{
    previous_ = current_;

    for (uint32_t i = 0; i < kKeyCount; ++i)
    {
        const int vk = virtualKeyFromEKey(static_cast<EKey>(i));
        current_[i] = (vk != 0) && ((GetAsyncKeyState(vk) & 0x8000) != 0);
    }

    const bool rmb_down = current_[static_cast<uint32_t>(EKey::kMouseRight)];
    if (rmb_down)
    {
        if (!rmb_was_down_)
        {
            ShowCursor(FALSE);
            // Discard first-frame delta to avoid jump on button press
            mouse_dx_ = 0.0f;
            mouse_dy_ = 0.0f;
        }
        else
        {
            mouse_dx_ = static_cast<float>(raw_dx_);
            mouse_dy_ = static_cast<float>(raw_dy_);
        }
    }
    else
    {
        if (rmb_was_down_)
        {
            ShowCursor(TRUE);
        }
        mouse_dx_ = 0.0f;
        mouse_dy_ = 0.0f;
    }

    // Reset raw accumulator AFTER reading, so deltas from pumpEvents() are not lost
    raw_dx_ = 0;
    raw_dy_ = 0;

    rmb_was_down_ = rmb_down;
}


void Win32Input::getMouseDelta(float& out_dx, float& out_dy) const
{
    out_dx = mouse_dx_;
    out_dy = mouse_dy_;
}


bool Win32Input::isKeyDown(EKey key) const
{
    const uint32_t idx = static_cast<uint32_t>(key);
    return (idx < kKeyCount) && current_[idx];
}


bool Win32Input::isKeyPressed(EKey key) const
{
    const uint32_t idx = static_cast<uint32_t>(key);
    return (idx < kKeyCount) && current_[idx] && !previous_[idx];
}


bool Win32Input::isKeyReleased(EKey key) const
{
    const uint32_t idx = static_cast<uint32_t>(key);
    return (idx < kKeyCount) && !current_[idx] && previous_[idx];
}


bool Win32Input::isActionDown(const std::string& action) const
{
    const auto it = action_map_.find(action);
    return (it != action_map_.end()) && isKeyDown(it->second);
}


bool Win32Input::isActionPressed(const std::string& action) const
{
    const auto it = action_map_.find(action);
    return (it != action_map_.end()) && isKeyPressed(it->second);
}


bool Win32Input::isActionReleased(const std::string& action) const
{
    const auto it = action_map_.find(action);
    return (it != action_map_.end()) && isKeyReleased(it->second);
}

// ---- Factory --------------------------------------------------------------

std::unique_ptr<IInput> IInput::create(const std::string& data_root_path,
                                       const std::string& config_name,
                                       WindowHandle window_handle)
{
    auto input = std::make_unique<Win32Input>();
    if (!input->init(data_root_path, config_name, static_cast<HWND>(window_handle)))
    {
        return nullptr;
    }
    return input;
}

}  // namespace platform
