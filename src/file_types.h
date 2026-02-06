#pragma once

#include <string_view>

namespace ft
{
enum class FileType
{
    CMake
};

constexpr std::string_view file_type_str(FileType type)
{
    switch (type)
    {
    case FileType::CMake:
        return "cmake";
        break;
    default:
        return "";
        break;
    }
}

constexpr FileType file_type_from_str(std::string_view str)
{
    if (file_type_str(FileType::CMake) == str)
    {
        return FileType::CMake;
    }
    else
    {
        throw "Invalid file type string";
    }
}
} // namespace ft