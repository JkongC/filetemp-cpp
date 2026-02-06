#pragma once

#include <any>
#include <argparse/argparse.hpp>
#include <stdexcept>
#include <unordered_map>

#include "arg_defs.h"

namespace std
{
template <>
struct hash<ft::ArgumentStringView>
{
    std::size_t operator()(const ft::ArgumentStringView &arg_sv) const noexcept
    {
        return hash<string_view>{}(arg_sv.full());
    }
};
} // namespace std

namespace ft
{

class ArgumentStorage
{
public:
    ArgumentStorage() = default;

    std::any &operator[](this ArgumentStorage &self, const ArgDef &arg) { return self.m_storage[arg]; }

    template <typename T = std::string>
    T &get(this ArgumentStorage &self, const ArgDef &arg)
    {
        auto &internal_any = self.m_storage[arg];
        if (internal_any.has_value())
        {
            return *std::any_cast<T>(&internal_any);
        }
        else
        {
            if constexpr (std::is_default_constructible_v<T>)
            {
                internal_any = T{};
                return *std::any_cast<T>(&internal_any);
            }
            else
            {
                throw std::logic_error{ "T is not default construtible." };
            }
        }
    }

    template <typename T>
    T get_val(this ArgumentStorage &self, const ArgDef &arg)
    {
        return self.get<T>(arg);
    }

    template <typename T>
    std::optional<T *> try_find(this const ArgumentStorage &self, const ArgDef &arg)
    {
        if (auto it = self.m_storage.find(arg); it != self.m_storage.end())
        {
            T *ptr = std::any_cast<T>(&*it);
            if (ptr)
            {
                return ptr;
            }
        }

        return std::nullopt;
    }

    auto &get_map(this ArgumentStorage &self) { return self.m_storage; }

private:
    std::unordered_map<std::string_view, std::any> m_storage;
};
} // namespace ft