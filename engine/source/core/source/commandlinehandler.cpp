#include "utility/commandlinehandler.h"

#include <algorithm>
#include <iterator>
#include <utility>


namespace utility {

CommandLineHandler::CommandLineHandler(std::vector<std::wstring> argv)
	: arguments_(std::move(argv))
{
	if (!arguments_.empty())
	{
		executable_name_ = std::move(arguments_.front());
		arguments_.erase(arguments_.begin());
	}
}


bool CommandLineHandler::consumeFlag(std::wstring_view flag)
{
	const std::wstring flag_name(flag);
	const auto options_end = std::find(arguments_.begin(), arguments_.end(), L"--");
	const auto new_end = std::remove(arguments_.begin(), options_end, flag_name);
	if (new_end == options_end)
	{
		return false;
	}

	arguments_.erase(new_end, options_end);
	return true;
}


ECommandLineOptionResult CommandLineHandler::consumeOption(std::wstring_view option, std::wstring& value)
{
	const auto options_end = std::find(arguments_.begin(), arguments_.end(), L"--");
	const auto option_it = std::find(arguments_.begin(), options_end, option);
	if (option_it == options_end)
	{
		return ECommandLineOptionResult::kAbsent;
	}

	const auto value_it = std::next(option_it);
	if (value_it == options_end || value_it->empty() || value_it->starts_with(L"--"))
	{
		return ECommandLineOptionResult::kMissingValue;
	}
	if (std::find(std::next(value_it), options_end, option) != options_end)
	{
		return ECommandLineOptionResult::kDuplicate;
	}

	value = std::move(*value_it);
	arguments_.erase(option_it, std::next(value_it));
	return ECommandLineOptionResult::kFound;
}

}  // namespace utility
