#include "arg/arg_basic.h"
#include "arg/arg_def.h"
#pragma warning(disable : 4996)
#include "arg/args.h"

#include <exception>
#include <filesystem>
#include <format>

#include <sstream>
#include <yaml-cpp/yaml.h>

#include "argparse/argparse.hpp"
#include "file_io.hpp"
#include "cmake_gen.h"
#include "log.hpp"

using namespace ft;

constexpr std::string_view c_fileName = "main.c";
constexpr std::string_view cxx_fileName = "main.cpp";

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
    YAML::Node &cache;

    template <typename T>
    void do_include(Arg<T> &arg)
    {
        if (parser.is_used(arg.full_name()))
        {
            return;
        }

        try
        {
            arg.assign(cache[arg.name()].template as<T>());
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
    void do_save(const Arg<T> &arg)
    {
        cache[arg.name()] = *arg;
    }
};

namespace ft
{
CMakeCacher::CMakeCacher(argparse::ArgumentParser &parser) noexcept
    : r_parser(parser)
{
    std::string_view cfg;
    YAML::Node cfg_cache;
    bool use_config = false;
    if (auto used_cfg = parser.present(Args::CMAKE_USECONFIG.full_name()))
    {
        cfg = used_cfg.value();
        use_config = true;
    }

    if (use_config || parser.present(Args::CMAKE_SAVEAS.full_name()))
    {
        try
        {
#ifdef FT_PLATFORM_WINDOWS
            const char *cache_root = std::getenv("LOCALAPPDATA");
#elifdef FT_PLATFORM_UNIX
            const char *cache_root = std::getenv("HOME");
#else
#error "System not supported."
#endif
            if (cache_root)
            {
                m_cachePath = cache_root;
                (m_cachePath /= ".filetemp") /= "cmake.yaml";
            }
            m_cache = YAML::LoadFile(m_cachePath.string());
        }
        catch (std::exception &)
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

    CacheIO includer{ parser, cfg_cache };

    includer.do_include(Args::CMAKE_VERSION);
    includer.do_include(Args::CMAKE_CSTD);
    includer.do_include(Args::CMAKE_CXXSTD);
    includer.do_include(Args::CMAKE_EXPORTCMD);
    includer.do_include(Args::CMAKE_MAINLANG);
}

void CMakeCacher::update()
{
    YAML::Node save_cache;
    std::string cfg;
    if (auto saved_cfg = r_parser.present(Args::CMAKE_SAVEAS.full_name()))
    {
        cfg = saved_cfg.value();
    }
    else
    {
        return;
    }
    save_cache = m_cache[std::string_view{ cfg }];

    CacheIO saver{ r_parser, save_cache };

    saver.do_save(Args::CMAKE_VERSION);
    saver.do_save(Args::CMAKE_CSTD);
    saver.do_save(Args::CMAKE_CXXSTD);
    saver.do_save(Args::CMAKE_EXPORTCMD);
    saver.do_save(Args::CMAKE_MAINLANG);

    auto cache_open_result = File::create(m_cachePath, FileMode::write);
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

struct CMakeOutput::Impl
{
    std::filesystem::path m_directory;
    std::string m_projName;
    std::string m_version;
    ArgType(Args::CMAKE_CSTD) m_cstd;
    ArgType(Args::CMAKE_CXXSTD) m_cxxstd;

    Impl() noexcept {}

    bool ensure_dir_valid_and_exists()
    {
        if (!std::filesystem::exists(m_directory))
        {
            std::error_code ec;
            std::filesystem::create_directories(m_directory, ec);
            if (ec)
            {
                WithSourceLocation{}.log_err("Fail to create directory \"{}\"", m_directory.string());
                return false;
            }
        }
        else if (!std::filesystem::is_directory(m_directory))
        {
            WithSourceLocation{}.log_err("Not a directory: \"{}\"", m_directory.string());
            return false;
        }

        return true;
    }

    bool output()
    {
        m_directory = *Args::CMAKE_WORKDIRECTORY;
        m_cstd = *Args::CMAKE_CSTD;
        m_cxxstd = *Args::CMAKE_CXXSTD;
        m_projName = *Args::CMAKE_PROJECT;
        m_version = *Args::CMAKE_VERSION;

        if (!ensure_dir_valid_and_exists())
        {
            return false;
        }

        auto file_create_result = File::create(m_directory / "CMakeLists.txt", FileMode::write);
        if (!file_create_result)
        {
            log_err("Failed to create CMakeLists.txt.");
            return false;
        }

        auto &file = file_create_result.value();

        std::string_view filename;
        std::string_view src;
        std::string_view export_command = *Args::CMAKE_EXPORTCMD ? "\nset(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n" : "";

        if (*Args::CMAKE_MAINLANG == "C")
        {
            filename = c_fileName;
            src = c_example;
        }
        else
        {
            filename = cxx_fileName;
            if (m_cxxstd >= 23)
            {
                src = cxx23_example;
            }
            else
            {
                src = cxx_example;
            }
        }

        std::string output =
            std::format(cmake_template, m_version, m_cstd, m_cxxstd, m_projName, filename, export_command);

        auto output_write_result = file.write(output);
        if (!output_write_result)
        {
            log_err("Failed to write into CMakeLists.txt.");
            return false;
        }

        if (*Args::CMAKE_SHOW)
        {
            std::ignore = file.flush_to(std::cout);
        }

        if (*Args::CMAKE_GENSRC)
        {
            auto src_path = m_directory / "src";
            if (!std::filesystem::create_directories(src_path))
            {
                log_err("Failed to create directories for source files.");
                return true;
            }

            auto src_create_result = File::create(src_path / filename, FileMode::write);
            if (!src_create_result)
            {
                log_err("Failed to create source files, you may have empty directories.");
                return true;
            }

            auto &src_file = src_create_result.value();
            auto src_write_result = src_file.write(src);
            if (!src_write_result)
            {
                log_err("Failed to write into source, you may have an empty source file.");
            }
            return true;
        }

        return true;
    }
};

bool CMakeOutput::output()
{
    return m_impl->output();
}

CMakeOutput::CMakeOutput() noexcept
    : m_impl(std::make_unique<Impl>())
{
}

CMakeOutput::~CMakeOutput() {}
} // namespace ft