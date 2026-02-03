#pragma once

#include <argparse/argparse.hpp>
#include <concepts>

namespace ft
{
struct ArgumentStringView
{
    std::string_view full;

    template <std::convertible_to<std::string_view> T>
    consteval ArgumentStringView(T arg)
    {
        std::string_view arg_{ arg };
        if (!arg_.starts_with("--"))
        {
            throw "Argument with invalid prefix.";
        }

        full = arg_;
    }

    constexpr std::string_view get_name(this const ArgumentStringView &self)
    {
        std::string_view ret{ self.full };
        ret.remove_prefix(2);
        return ret;
    }
};
} // namespace ft