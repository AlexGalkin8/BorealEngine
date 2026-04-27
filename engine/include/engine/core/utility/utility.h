#pragma once

#include <string>

#define SAFE_DELETE(obj) if (obj) { delete (obj); (obj) = nullptr; }

namespace utility {

class NonCopyable
{
protected:
	NonCopyable() = default;
	~NonCopyable() = default;

public:
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};


#define ENUM_CLASS_BITWISE_OPERATORS(T) \
	inline constexpr T operator|(T a, T b)         \
	{                                              \
		return (T)((uint64_t)a | (uint64_t)b);     \
	}                                              \
	inline constexpr T operator&(T a, T b)         \
	{                                              \
		return (T)((uint64_t)a & (uint64_t)b);     \
	}                                              \
	inline constexpr T operator^(T a, T b)         \
	{                                              \
		return (T)((uint64_t)a ^ (uint64_t)b);     \
	}                                              \
	inline constexpr T operator~(T a)              \
	{                                              \
		return (T) ~(uint64_t)a;                   \
	}                                              \
	inline T& operator|=(T& a, T b)                \
	{                                              \
		return a = (T)((uint64_t)a | (uint64_t)b); \
	}                                              \
	inline T& operator&=(T& a, T b)                \
	{                                              \
		return a = (T)((uint64_t)a & (uint64_t)b); \
	}                                              \
	inline T& operator^=(T& a, T b)                \
	{                                              \
		return a = (T)((uint64_t)a ^ (uint64_t)b); \
	}


extern std::wstring translateStringToWString(std::string_view str);
extern std::string translateWStringToString(std::wstring_view str);

extern std::wstring getWinApiLastErrorString();


}  // namespace utility
