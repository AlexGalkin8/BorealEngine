#include "engine/platform/window.h"

#include <engine/core/utility/assert.h>
#include <engine/core/utility/utility.h>
#include <engine/platform/input.h>

#include <windows.h>
#include <string>
#include <vector>

namespace platform {

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

std::unique_ptr<IWindow> IWindow::create(const Desc& desc)
{
	std::wstring class_name = desc.name + L" window class";

	HINSTANCE hinstance = NULL;
	ASSERT_MSG_RETURN_NULL(GetModuleHandleEx(0, NULL, &hinstance),
		L"Error getting window handle. Message: %s", utility::getWinApiLastErrorString().c_str());

	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.hIconSm = NULL;
	windowClass.lpfnWndProc = (WNDPROC)WindowProc;
	windowClass.lpszClassName = class_name.c_str();
	windowClass.hInstance = hinstance;
	windowClass.lpszMenuName = NULL;
	windowClass.style = 0;
	windowClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = NULL;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;

	ASSERT_MSG_RETURN_NULL(RegisterClassEx(&windowClass),
		L"Failed to register window class. Message: %s", utility::getWinApiLastErrorString().c_str());

	RECT window_rect = { 0, 0, static_cast<LONG>(desc.width), static_cast<LONG>(desc.height) };
	ASSERT_MSG_RETURN_NULL(AdjustWindowRectEx(&window_rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW),
		"Error on adjust window rect");

	HWND handle = CreateWindowEx(
		0,                   // Optional window styles.
		class_name.c_str(),  // Window class
		desc.name.c_str(),   // Window text
		WS_OVERLAPPEDWINDOW, // Window style
		CW_USEDEFAULT,       // x
		CW_USEDEFAULT,       // y
		CW_USEDEFAULT,       // width
		CW_USEDEFAULT,       // height
		NULL,                // Parent window
		NULL,                // Menu
		hinstance,           // Instance handle
		NULL                 // Additional application data
	);

	ASSERT_MSG_RETURN_NULL(handle, L"Failed to create window. Message: %s", utility::getWinApiLastErrorString().c_str());

	auto window = std::make_unique<IWindow>(desc, handle);
	SetWindowLongPtr(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window.get()));

	ShowWindow(handle, SW_SHOW);

	return window;
}


IWindow::IWindow(const Desc& desc, WindowHandle handle)
	: handle_(handle)
	, desc_(desc)
{
}


IWindow::~IWindow()
{
	HWND hwnd = static_cast<HWND>(handle_);
	if (!hwnd)
	{
		return;
	}

	if (!IsWindow(hwnd))
	{
		handle_ = nullptr;
		return;
	}

	if (!DestroyWindow(hwnd))
	{
		ASSERT_FAIL(L"Failed to close window. Error code: %s", utility::getWinApiLastErrorString().c_str());
	}

	handle_ = nullptr;
}



bool IWindow::pumpEvents()
{
	MSG msg{};
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}


bool IWindow::isActive() const
{
	HWND hwnd = static_cast<HWND>(handle_);
	if (!hwnd)
	{
		return false;
	}

	return (GetForegroundWindow() == hwnd) && !IsIconic(hwnd);
}


void* IWindow::getHandle() const
{
	return handle_;
}


uint32_t IWindow::getWidth() const
{
	return desc_.width;
}


uint32_t IWindow::getHeight() const
{
	return desc_.height;
}


bool IWindow::consumeResize(uint32_t& out_width, uint32_t& out_height)
{
	if (!has_pending_resize_)
	{
		return false;
	}

	has_pending_resize_ = false;
	out_width = desc_.width;
	out_height = desc_.height;
	return true;
}


void IWindow::resize(uint32_t new_width, uint32_t new_height)
{
	desc_.width = new_width;
	desc_.height = new_height;
	has_pending_resize_ = true;
}


static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INPUT:
	{
		UINT size = 0;
		if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) == 0 && size > 0)
		{
			std::vector<BYTE> buf(size);
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buf.data(), &size, sizeof(RAWINPUTHEADER)) == size)
			{
				const RAWINPUT* raw = reinterpret_cast<const RAWINPUT*>(buf.data());
				if (raw->header.dwType == RIM_TYPEMOUSE && (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
				{
					notifyRawMouseInput(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
				}
			}
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	case WM_CREATE:
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_MOVE:
		break;
	case WM_SIZE:
	{
		if (wParam != SIZE_MINIMIZED)
		{
			IWindow* window = reinterpret_cast<IWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (window)
			{
				window->resize(static_cast<uint32_t>(LOWORD(lParam)), static_cast<uint32_t>(HIWORD(lParam)));
			}
		}
		break;
	}
	case WM_DISPLAYCHANGE:
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps{};
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		return 0;
	}
	case WM_ERASEBKGND:
		return 1;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}
	return 0;
}

}  // namespace platform
