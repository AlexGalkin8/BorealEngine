#include "utility/enum.h"

#include <unordered_map>


namespace utility {

template <typename T>
std::span<const EnumMapping<T, const char*>> enumValues()
{
    static_assert(!std::is_same_v<T, int>,
        "you should either define EnumName<T> variable for sequence of enum values or override this function for your enum");

    return { EnumName<T> }; // we are return enum names array by default
}

template <typename T>
bool enumFromString(const std::string& str, T& value)
{
    static_assert(!std::is_same_v<T, int>,
        "you should either define EnumName<T> variable for sequence of enum values or override this function for your enum");

    if constexpr (ArraySize(EnumName<T>) <= 12)
    {
        for (const auto& button : EnumName<T>)
        {
            if (str == button.value)
            {
                value = button.key;
                return true;
            }
        }

        return false;
    }
    else
    {
        // values are too many, we'll use hashmap instead

        // init hash map via lambda here
        static auto map = []()
        {
            std::unordered_map<std::string, T> value_map;
            for (const auto& e : EnumName<T>)
            {
                value_map[e.value] = e.key;
            }

            return value_map;
        }();

        auto it = map.find(str);
        if (it != map.end())
        {
            value = it->second;
            return true;
        }
        return false;
    }
}


template <typename T>
std::string enumToString(T value)
{
    static_assert(!std::is_same_v<T, int>,
        "you should either define EnumName<T> variable for sequence of enum values or override this function for your enum");

    if (static_cast<size_t>(value) < ArraySize(EnumName<T>))
        return EnumName<T>[static_cast<size_t>(value)].value;

    return "";
}

}  // namespace utility
