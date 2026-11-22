#pragma once

#include <string>
#include <string_view>
#include <vector>


namespace utility {

enum class ECommandLineOptionResult
{
	kAbsent,
	kFound,
	kMissingValue,
	kDuplicate,
};


class CommandLineHandler
{
public:
	explicit CommandLineHandler(std::vector<std::wstring> argv);

	const std::wstring& getExecutableName() const { return executable_name_; }
	const std::vector<std::wstring>& getRemainingArguments() const { return arguments_; }

	bool consumeFlag(std::wstring_view flag);
	ECommandLineOptionResult consumeOption(std::wstring_view option, std::wstring& value);

private:
	std::wstring executable_name_;
	std::vector<std::wstring> arguments_;
};

}  // namespace utility
