#include <cstdarg>
#include <exception>
#include <filesystem>
#include <format>

#include <sstream>
#include <yaml-cpp/yaml.h>

#include "arg_defs.h"
#include "argparse/argparse.hpp"
#include "args.hpp"
#include "file_io.hpp"
#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"
#include "cmake_gen.h"
#include "log.hpp"

using namespace ft;

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

struct CacheIO
{
    argparse::ArgumentParser &parser;
    ArgumentStorage &args;
    YAML::Node &cache;

    template <typename T>
    void do_include(const ArgDef &arg)
    {
        if (parser.is_used(arg))
        {
            return;
        }

        try
        {
            args.get<T>(arg) = cache[arg.name.name()].as<T>();
        }
        catch (const YAML::InvalidNode &)
        {
            return;
        }
        catch (const YAML::BadConversion &)
        {
            log_err("Cache file corrupted, arguments may not work as expected.");
        }
    }

    template <typename T>
    void do_save(const ArgDef &arg)
    {
        cache[arg.name.name()] = args.get<T>(arg);
    }
};

namespace ft
{
CMakeCacher::CMakeCacher(argparse::ArgumentParser &parser, ArgumentStorage &args) noexcept
    : r_parser(parser)
    , r_args(args)
{
    std::string_view cfg;
    YAML::Node cfg_cache;
    bool use_config = false;
    if (auto used_cfg = parser.present(Arg::CMAKE_USECONFIG))
    {
        cfg = used_cfg.value();
        use_config = true;
    }

    if (use_config || parser.present(Arg::CMAKE_SAVEAS))
    {
        try
        {
            m_cache = YAML::LoadFile("./cache/cmake.yaml");
        }
        catch ([[maybe_unused]] std::exception &e)
        {
            if (use_config)
            {
                log_err("Failed to load cache file, config related options may not work as expected.");
            }
            m_cache = YAML::Node{};
            return;
        }
    }
    else
    {
        return;
    }

    cfg_cache = m_cache[cfg];

    CacheIO includer{ parser, args, cfg_cache };

#define include_cache(arg) includer.do_include<ArgRepr<arg.ident>>(arg)

    include_cache(Arg::CMAKE_VERSION);
    include_cache(Arg::CMAKE_CSTD);
    include_cache(Arg::CMAKE_CXXSTD);
    include_cache(Arg::CMAKE_EXPORTCMD);
    include_cache(Arg::CMAKE_MAINLANG);

#undef include_cache
}

void CMakeCacher::update()
{
    YAML::Node save_cache;
    std::string cfg;
    if (auto saved_cfg = r_parser.present(Arg::CMAKE_SAVEAS))
    {
        cfg = saved_cfg.value();
    }
    else
    {
        return;
    }
    save_cache = m_cache[std::string_view{ cfg }];

    CacheIO saver{ r_parser, r_args, save_cache };

#define save_cache(arg) saver.do_save<ArgRepr<arg.ident>>(arg)

    save_cache(Arg::CMAKE_VERSION);
    save_cache(Arg::CMAKE_CSTD);
    save_cache(Arg::CMAKE_CXXSTD);
    save_cache(Arg::CMAKE_EXPORTCMD);
    save_cache(Arg::CMAKE_MAINLANG);

#undef save_cache
    auto cache_open_result = File::create("./cache/cmake.yaml", FileMode::write);
    if (!cache_open_result)
    {
        log_err("Failed to save cache, save-as may not work as expected.");
        return;
    }

    File &cache_file = cache_open_result.value();
    std::stringstream yaml_result;
    yaml_result << m_cache;

    auto cache_write_result = cache_file.write(yaml_result.view());
    if (!cache_write_result)
    {
        log_err("Failed to write into cache file, save-as may not work as expected.");
    }
}

bool CMakeOutput::output()
{
    using CSTDT = ArgRepr<Arg::CMAKE_CSTD.ident>;
    using CXXSTDT = ArgRepr<Arg::CMAKE_CXXSTD.ident>;

    std::filesystem::path directory = r_args.get(Arg::CMAKE_WORKDIRECTORY);
    CSTDT cstd = r_args.get<CSTDT>(Arg::CMAKE_CSTD);
    CXXSTDT cxxstd = r_args.get<CXXSTDT>(Arg::CMAKE_CXXSTD);
    std::string proj_name = r_args.get(Arg::CMAKE_PROJECT);
    std::string version = r_args.get(Arg::CMAKE_VERSION);

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

    auto file_create_result = File::create(directory / "CMakeLists.txt", FileMode::write);
    if (!file_create_result)
    {
        log_err("Failed to create CMakeLists.txt.");
        return false;
    }

    auto &file = file_create_result.value();

    std::string_view ext;
    std::string_view src;
    std::string_view export_command =
        r_args.get<bool>("--export-commands") ? "\nset(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n" : "";

    if (r_args.get("--main-lang") == "C")
    {
        ext = c_extension;
        src = c_example;
    }
    else
    {
        ext = cxx_extension;
        if (cxxstd >= 23)
        {
            src = cxx23_example;
        }
        else
        {
            src = cxx_example;
        }
    }

    std::string output = std::format(cmake_template, version, cstd, cxxstd, proj_name, ext, export_command);

    auto output_write_result = file.write(output);
    if (!output_write_result)
    {
        log_err("Failed to write into CMakeLists.txt.");
        return false;
    }

    if (r_args.get<bool>("--show"))
    {
        std::ignore = file.flush_to(std::cout);
    }

    if (r_args.get<bool>("--generate-src"))
    {
        auto src_path = directory / "src";
        if (std::filesystem::create_directories(src_path))
        {
            auto src_create_result = File::create(src_path / (std::string{ "main." }.append(ext)), FileMode::write);
            if (src_create_result)
            {
                auto &src_file = src_create_result.value();
                auto src_write_result = src_file.write(src);
                if (!src_write_result)
                {
                    log_err("Failed to write into source, you may have an empty source file.");
                }
                return true;
            }
        }
        log_err("Failed to generate source.");
    }

    return true;
}
} // namespace ft