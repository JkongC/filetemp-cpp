#include "gen.h"
#include "argparse/argparse.hpp"
#include "cmake_gen.h"
#include <memory>

namespace ft
{
Output Output::create(FileType type, ArgumentStorage &args)
{
    switch (type)
    {
    case FileType::CMake:
        return Output{ std::make_unique<detail::OutputAdapter<CMakeOutput>>(args) };
        break;
    default:
        throw;
        break;
    }
}

bool Output::output()
{
    return m_base->do_output();
}

ScopeCacher ScopeCacher::create(FileType type, argparse::ArgumentParser &parser, ArgumentStorage &args)
{
    switch (type)
    {
    case FileType::CMake:
        return ScopeCacher{ std::make_unique<detail::CacherAdapter<CMakeCacher>>(parser, args) };
        break;
    default:
        throw;
        break;
    }
}
} // namespace ft