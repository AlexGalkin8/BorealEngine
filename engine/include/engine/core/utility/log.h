#pragma once

#include <format>
#include <source_location>
#include <string_view>


namespace logging {

enum class ELogLevel
{
	kInfo,
	kWarning,
	kError,
};


void write(ELogLevel level, std::string_view category, std::string_view message,
	const std::source_location& location = std::source_location::current());

}  // namespace logging


#define LOG_INFO(category, ...) ::logging::write(::logging::ELogLevel::kInfo, (category), ::std::format(__VA_ARGS__))
#define LOG_WARNING(category, ...) ::logging::write(::logging::ELogLevel::kWarning, (category), ::std::format(__VA_ARGS__))
#define LOG_ERROR(category, ...) ::logging::write(::logging::ELogLevel::kError, (category), ::std::format(__VA_ARGS__))
