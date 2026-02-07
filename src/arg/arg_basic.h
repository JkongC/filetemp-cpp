#pragma once

#include <concepts>
#include <string> // IWYU pragma: export
#include <string_view>

namespace ft
{
struct ArgumentStringView
{
    consteval ArgumentStringView(std::string_view name_, std::string_view short_name_ = "")
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

template <std::copy_constructible T>
struct Arg
{
    ArgumentStringView m_name;
    T m_content{};

    Arg(ArgumentStringView name_)
        : m_name(name_)
    {
    }

    std::string_view full_name(this const Arg &self) { return self.m_name.full(); }
    std::string_view name(this const Arg &self) { return self.m_name.name(); }
    std::string_view short_name(this const Arg &self) { return self.m_name.short_name(); }

    const T &operator*(this const Arg &self) { return self.m_content; }
    T &operator&(this Arg &self) { return self.m_content; }

    void assign(this Arg &self, const T &val) { self.m_content = val; }
};

#define ArgType(arg) decltype(arg.m_content)

Arg(ArgumentStringView) -> Arg<std::string>;
} // namespace ft