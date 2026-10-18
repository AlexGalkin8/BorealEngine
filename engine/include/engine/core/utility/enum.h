#pragma once

#include <string>
#include <span>

namespace utility {

template <typename Key, typename Value>
struct EnumMapping
{
	Key   key;
	Value value;
};

template <const auto& array, size_t idx = ArraySize(array) - 1>
struct CompileCheck : CompileCheck<array, idx - 1>
{
	static_assert(static_cast<size_t>(array[idx].key) == idx, "order mismatch");
};

template <const auto& array>
struct CompileCheck<array, 0>
{
	static_assert(static_cast<size_t>(array[0].key) == 0, "order mismatch");
};


template <typename T>
inline constexpr EnumMapping<T, const char*> EnumName[1] = { };

template <typename T>
extern std::span<const EnumMapping<T, const char*>> enumValues();

template <typename T>
extern bool enumFromString(const std::string& str, T& value);

template <typename T>
extern std::string enumToString(T value);

}  // namespace utility


#define CHECK_MAPPING_ORDER(ref) static_assert(sizeof(CompileCheck<ref>) == 1)
