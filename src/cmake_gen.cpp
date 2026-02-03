#include <filesystem>
#include <format>

#include <sstream>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

#include "argparse/argparse.hpp"
#include "args.hpp"
#include "file_io.hpp"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"
#include "cmake_gen.h"
#include "log.hpp"

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

static bool initialize_cache(argparse::ArgumentParser &args,
                             YAML::Node &cache,
                             YAML::Node &config_cache,
                             std::string &used_config,
                             std::string &save_config)
{
    if (auto cfg_opt = args.present("--use-config"))
    {
        used_config = cfg_opt.value();
        try
        {
            cache = YAML::LoadFile("cache/cmake.yaml");
            config_cache = cache[used_config];
            if (!config_cache)
            {
                WithSourceLocation{}.log_err("Invalid config \"{}\".", used_config);
                return false;
            }
        }
        catch (std::runtime_error &err)
        {
            WithSourceLocation{}.log_err("Failed to load cache file: {}", err.what());
            return false;
        }
    }
    else if (auto cfg_opt = args.present("--save-as"))
    {
        save_config = cfg_opt.value();
        try
        {
            cache = YAML::LoadFile("cache/cmake.yaml");
        }
        catch (std::runtime_error &err)
        {
            cache = YAML::Node{};
        }
    }

    return true;
}

bool gen_cmake_file(argparse::ArgumentParser &args)
{
    using namespace argparse;

    YAML::Node cache{};
    YAML::Node config_cache{};
    std::string used_config;
    std::string save_config;

    if (!initialize_cache(args, cache, config_cache, used_config, save_config))
    {
        return false;
    }

    auto place_arg = [&]<typename T>(ArgumentStringView arg, T &obj)
    {
        if (args.is_used(arg.full) || !config_cache || !config_cache[arg.get_name()])
        {
            obj = args.get<T>(arg.full);
            return;
        }

        try
        {
            obj = config_cache[arg.get_name()].as<T>();
        }
        catch (std::runtime_error &err)
        {
            WithSourceLocation{}.log_err("Failed to read from cache: {}", err.what());
            log_info("Using default value for \"{}\"", arg.full);
            obj = args.get<T>(arg.full);
        }
    };

    std::filesystem::path directory = args.get("directory");
    int cstd;
    int cxxstd;
    std::string proj_name;
    std::string version;

    place_arg("--cstd", cstd);
    place_arg("--cxxstd", cxxstd);
    place_arg("--project", proj_name);
    place_arg("--version", version);

    if (!std::filesystem::exists(directory))
    {
        std::error_code ec;
        std::filesystem::create_directories(directory, ec);
        if (ec)
        {
            WithSourceLocation{}.log_err("Fail to create directory \"{}\"", directory.string());
            return false;
        }
    }
    else if (!std::filesystem::is_directory(directory))
    {
        WithSourceLocation{}.log_err("Not a directory: \"{}\"", directory.string());
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

    std::ignore = file.write(std::format(cmake_template, version, cstd, cxxstd, proj_name, ext, export_command));

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

    constexpr ArgumentStringView saved_args[]{ "--version", "--cstd", "--cxxstd", "--export-commands", "--main-lang" };

    if (auto save_as_opt = args.present("--save-as"))
    {
        auto &save_config = save_as_opt.value();
        for (const auto &sa : saved_args)
        {
            if (args.is_used(sa.full))
            {
                if (sa.full == "--cstd" || sa.full == "--cxxstd")
                {
                    cache[save_config][sa.get_name()] = args.get<int>(sa.full);
                }
                else if (sa.full == "--export-commands")
                {
                    cache[save_config][sa.get_name()] = args.get<bool>(sa.full);
                }
                else
                {
                    cache[save_config][sa.get_name()] = args.get<std::string>(sa.full);
                }
            }
        }

        if (!std::filesystem::exists("./cache"))
        {
            std::filesystem::create_directories("./cache");
        }

        auto cache_open_result = File::create("./cache/cmake.yaml", FileMode::write);
        if (cache_open_result)
        {
            std::stringstream stream;
            stream << cache;
            if (!cache_open_result.value().write(stream.str()))
            {
                WithSourceLocation{}.log_err("Failed to save config \"{}\".", used_config);
            }
        }
        else
        {
            WithSourceLocation{}.log_err("Failed to save to config file.");
        }
    }

    return true;
}
} // namespace ft