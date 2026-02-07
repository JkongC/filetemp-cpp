#pragma once
#include <memory>

#include <argparse/argparse.hpp>
#include <yaml-cpp/yaml.h>

namespace ft
{
class CMakeOutput
{
public:
    CMakeOutput() noexcept;
    ~CMakeOutput();

    bool output();

    struct Impl;

private:
    std::unique_ptr<Impl> m_impl;
};

class CMakeCacher
{
public:
    CMakeCacher(argparse::ArgumentParser &parser) noexcept;

    void update();

private:
    argparse::ArgumentParser &r_parser;
    YAML::Node m_cache;
    std::filesystem::path m_cachePath;
};
} // namespace ft