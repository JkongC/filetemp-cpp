#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace ft
{
struct ArgumentStringView
{
    consteval ArgumentStringView(std::string_view name_, std::string_view short_name_)
    {
        constexpr auto prefix_count = [](std::string_view n) consteval
        {
            std::size_t prefix_char_count = 0;
            for (auto &&c : n)
            {
                if (c == '-')
                {
                    ++prefix_char_count;
                }
                else
                {
                    break;
                }
            }

            return prefix_char_count;
        };

        std::size_t name_prefix_count = prefix_count(name_);
        std::size_t short_prefix_count = prefix_count(short_name_);

        if (name_prefix_count != 0 && name_prefix_count != 2)
        {
            throw "Invalid argument name";
        }

        if (short_name_.size() != 0 && short_prefix_count != 1)
        {
            throw "Invalid argument short name";
        }

        m_optional = name_prefix_count == 2;
        m_fullName = name_;
        m_shortName = short_name_;
    }

    // Full name WITH prefix
    constexpr std::string_view full(this const ArgumentStringView &self) { return self.m_fullName; }

    // Full name WITHOUT prefix
    constexpr std::string_view name(this const ArgumentStringView &self)
    {
        if (self.m_optional)
        {
            std::string_view ret{ self.m_fullName };
            ret.remove_prefix(2);
            return ret;
        }
        else
        {
            return self.m_fullName;
        }
    }

    // Short name WITHOUT prefix
    constexpr std::string_view short_name(this const ArgumentStringView &self)
    {
        std::string_view ret{ self.m_shortName };
        ret.remove_prefix(1);
        return ret;
    }

    constexpr operator std::string_view(this const ArgumentStringView &self) { return self.m_fullName; }

    constexpr bool operator==(this const ArgumentStringView &self, const ArgumentStringView &other) noexcept
    {
        return self.m_fullName == other.m_fullName;
    }

private:
    std::string_view m_fullName;
    std::string_view m_shortName;
    bool m_optional;
};

namespace detail
{
    consteval std::uintmax_t arg_ident(std::string_view arg_name)
    {
        auto hasher = [=](std::uintmax_t base)
        {
            std::uintmax_t ret = 0;
            for (const char &c : arg_name)
            {
                ret = ret * base + static_cast<std::uintmax_t>(c);
            }
            return ret;
        };
        return hasher(131) ^ (hasher(129) >> 47) + arg_name.size();
    }
} // namespace detail

struct ArgDef
{
    ArgumentStringView name;
    std::uintmax_t ident;

    consteval ArgDef(std::string_view name_, std::string_view short_name = "")
        : name(name_, short_name)
        , ident(detail::arg_ident(name_))
    {
    }

    consteval ArgDef(const char *name_)
        : ArgDef(std::string_view{ name_ })
    {
    }

    constexpr operator std::string_view(this const ArgDef &self) { return self.name; }
};

namespace Arg
{
    constexpr ArgDef CMAKE_WORKDIRECTORY = "directory";
    constexpr ArgDef CMAKE_VERSION = { "--version", "-v" };
    constexpr ArgDef CMAKE_CSTD = { "--cstd", "-c" };
    constexpr ArgDef CMAKE_CXXSTD = { "--cxxstd", "-C" };
    constexpr ArgDef CMAKE_PROJECT = { "--project", "-p" };
    constexpr ArgDef CMAKE_MAINLANG = { "--main-lang", "-m" };
    constexpr ArgDef CMAKE_SAVEAS = { "--save-as", "-S" };
    constexpr ArgDef CMAKE_USECONFIG = { "--use-config", "-U" };
    constexpr ArgDef CMAKE_EXPORTCMD = { "--export-commands", "-e" };
    constexpr ArgDef CMAKE_GENSRC = { "--generate-src", "-g" };
    constexpr ArgDef CMAKE_SHOW = { "--show", "-s" };

    constexpr inline auto INT_TYPED_ARGS = std::to_array<ArgDef>({ Arg::CMAKE_CSTD, Arg::CMAKE_CXXSTD });
    constexpr inline auto BOOL_TYPED_ARGS =
        std::to_array<ArgDef>({ Arg::CMAKE_EXPORTCMD, Arg::CMAKE_GENSRC, Arg::CMAKE_SHOW });
} // namespace Arg

template <std::size_t N>
constexpr bool match_typed_arg(const std::array<ArgDef, N> &typed_args, const ArgDef &arg)
{
    for (const ArgDef &targ : typed_args)
    {
        if (targ.ident == arg.ident)
        {
            return true;
        }
    }

    return false;
}

template <std::size_t N>
constexpr bool match_typed_arg(const std::array<ArgDef, N> &typed_args, std::uintmax_t ident)
{
    for (const ArgDef &targ : typed_args)
    {
        if (targ.ident == ident)
        {
            return true;
        }
    }

    return false;
}

namespace detail
{
    template <std::uintmax_t Ident>
    consteval auto decide_arg_repr()
    {
        if constexpr (match_typed_arg(Arg::INT_TYPED_ARGS, Ident))
        {
            return int{};
        }
        else if constexpr (match_typed_arg(Arg::BOOL_TYPED_ARGS, Ident))
        {
            return bool{};
        }
        else
        {
            return std::string{};
        }
    }
} // namespace detail

template <std::uintmax_t ArgIdent>
using ArgRepr = decltype(detail::decide_arg_repr<ArgIdent>());

} // namespace ft