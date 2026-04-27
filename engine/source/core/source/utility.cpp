#include "utility/utility.h"

#include <string>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#else
#include <locale>
#include <codecvt>
#endif

namespace utility {

std::wstring translateStringToWString(std::string_view str)
{
	if (str.empty()) return std::wstring();

#ifdef _WIN32
	int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		str.data(), static_cast<int>(str.size()),
		nullptr, 0);
	if (required == 0)
	{
		return std::wstring();
	}
	std::wstring out;
	out.resize(required);
	int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		str.data(), static_cast<int>(str.size()),
		&out[0], required);
	if (written == 0) return std::wstring();
	return out;
#else
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	try
	{
		return conv.from_bytes(str.data(), str.data() + str.size());
	} catch (...)
	{
		return std::wstring();
	}
#endif
}


std::string translateWStringToString(std::wstring_view str)
{
	if (str.empty())
	{
		return std::string{};
	}

	int required = WideCharToMultiByte(
		CP_UTF8,
		WC_ERR_INVALID_CHARS,
		str.data(),
		static_cast<int>(str.size()),
		nullptr,
		0,
		nullptr,
		nullptr);

	if (required == 0)
	{
		return {};
	}

	std::string result;
	result.resize(required);

	int written = WideCharToMultiByte(
		CP_UTF8,
		WC_ERR_INVALID_CHARS,
		str.data(),
		static_cast<int>(str.size()),
		&result[0],
		required,
		nullptr,
		nullptr);

	if (written == 0)
	{
		return {};
	}

	if (!result.empty() && result.back() == '\0')
	{
		result.pop_back();
	}
	else if (written < static_cast<int>(result.size()))
	{
		result.resize(written);
	}

	return result;
}


extern std::wstring getWinApiLastErrorString()
{
	DWORD err = GetLastError();
	if (err == 0)
	{
		return std::wstring{};
	}

	LPWSTR buffer = nullptr;
	DWORD size = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&buffer,
		0,
		nullptr);

	if (size <= 0 || buffer == nullptr)
	{
		wchar_t tmp[64];
		wsprintf(tmp, L"Unknown error 0x%08X", err);
		return std::wstring{ tmp };
	}
	while (size > 0 && (buffer[size - 1] == L'\n' || buffer[size - 1] == L'\r' || buffer[size - 1] == L' ' || buffer[size - 1] == L'\t'))
	{
		buffer[--size] = L'\0';
	}
	std::wstring message{ buffer, size };
	message = message.substr(0, message.find_first_of(L"\n\r\t"));
	LocalFree(buffer);

	return message;
}

}  // namespace utility
