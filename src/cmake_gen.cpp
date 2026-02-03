#include "cmake_gen.h"
#include "file_io.hpp"
#include <filesystem>
#include <format>

constexpr std::string_view c_extension = "c";
constexpr std::string_view cxx_extension = "cpp";

constexpr std::string_view c_example = R"(#include <stdio.h>
int main()
{
    printf("Hello World");
    return 0;
})";

constexpr std::string_view cxx_example = R"(#include <iostream>
int main()
{
    std::cout << "Hello World" << std::endl;
})";

constexpr std::string_view cxx23_example = R"(#include <print>
int main()
{
    std::println("Hello World");
})";

constexpr std::string_view cmake_template = R"(cmake_minimum_required(VERSION {0})

set(CMAKE_C_STANDARD {1})
set(CMAKE_CXX_STANDARD {2})
{5}
project({3})

add_executable({3})
target_sources({3} PRIVATE src/main.{4})
target_include_directories({3} PRIVATE src))";

namespace ft
{
bool gen_cmake_file(argparse::ArgumentParser &args)
{
    using namespace argparse;

    std::filesystem::path directory = args.get("directory");
    if (!std::filesystem::exists(directory))
    {
        std::error_code ec;
        std::filesystem::create_directories(directory, ec);
        if (ec)
        {
            log_err("Fail to create directory \"{}\"", directory.string());
            return false;
        }
    }
    else if (!std::filesystem::is_directory(directory))
    {
        log_err("Not a directory: \"{}\"", directory.string());
        return false;
    }

    auto file_create_result = File::create(directory / "CMakeLists.txt", FileMode::write, true);
    if (!check_file_op_result(file_create_result))
    {
        return false;
    }

    auto &file = file_create_result.value();

    std::string_view ext;
    std::string_view src;
    std::string_view export_command =
        args.get<bool>("--export-commands") ? "\nset(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n" : "";
    std::string proj_name = args.get("--project");

    if (args.get("--main-lang") == "C")
    {
        ext = c_extension;
        src = c_example;
    }
    else
    {
        ext = cxx_extension;
        if (args.get<int>("--cxxstd") >= 23)
        {
            src = cxx23_example;
        }
        else
        {
            src = cxx_example;
        }
    }

    std::ignore = file.write(std::format(cmake_template,
                                         args.get("--version"),
                                         args.get<int>("--cstd"),
                                         args.get<int>("--cxxstd"),
                                         args.get("--project"),
                                         ext,
                                         export_command));

    if (args.get<bool>("--show"))
    {
        std::ignore = file.flush_to(std::cout);
    }

    auto write_result = file.flush();
    if (!check_file_op_result(write_result))
    {
        return false;
    }

    if (args.get<bool>("--generate-src"))
    {
        auto src_path = directory / "src";
        if (std::filesystem::create_directories(src_path))
        {
            auto src_create_result = File::create(src_path / (std::string{ "main." }.append(ext)), FileMode::write);
            if (check_file_op_result(src_create_result))
            {
                auto &src_file = src_create_result.value();
                std::ignore = src_file.write(src);
            }
        }
    }

    return true;
}
} // namespace ft