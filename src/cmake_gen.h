#pragma once
#include <argparse/argparse.hpp>
#include <yaml-cpp/yaml.h>
#include "args.hpp"

namespace ft
{
class CMakeOutput
{
public:
    CMakeOutput(ArgumentStorage &args) noexcept
        : r_args(args)
    {
    }

    bool output();

private:
    ArgumentStorage &r_args;
};

class CMakeCacher
{
public:
    CMakeCacher(argparse::ArgumentParser &parser, ArgumentStorage &args) noexcept;

    void update();

private:
    argparse::ArgumentParser &r_parser;
    ArgumentStorage &r_args;
    YAML::Node m_cache;
    std::filesystem::path m_cachePath;
};
} // namespace ft