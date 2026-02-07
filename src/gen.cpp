#include <memory>

#include "gen.h"
#include "argparse/argparse.hpp"
#include "cmake_gen.h"

namespace ft
{
Output Output::create(FileType type)
{
    switch (type)
    {
    case FileType::CMake:
        return Output{ std::make_unique<detail::OutputAdapter<CMakeOutput>>() };
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

ScopeCacher ScopeCacher::create(FileType type, argparse::ArgumentParser &parser)
{
    switch (type)
    {
    case FileType::CMake:
        return ScopeCacher{ std::make_unique<detail::CacherAdapter<CMakeCacher>>(parser) };
        break;
    default:
        throw;
        break;
    }
}
} // namespace ft