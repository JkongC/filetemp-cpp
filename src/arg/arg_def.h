#pragma once

#include "arg_basic.h"

namespace ft
{
namespace Args
{
    inline Arg CMAKE_WORKDIRECTORY = ArgumentStringView{ "directory" };
    inline Arg CMAKE_VERSION = ArgumentStringView{ "--version", "-v" };
    inline Arg<int> CMAKE_CSTD = ArgumentStringView{ "--cstd", "-c" };
    inline Arg<int> CMAKE_CXXSTD = ArgumentStringView{ "--cxxstd", "-C" };
    inline Arg CMAKE_PROJECT = ArgumentStringView{ "--project", "-p" };
    inline Arg CMAKE_MAINLANG = ArgumentStringView{ "--main-lang", "-m" };
    inline Arg CMAKE_SAVEAS = ArgumentStringView{ "--save-as", "-S" };
    inline Arg CMAKE_USECONFIG = ArgumentStringView{ "--use-config", "-U" };
    inline Arg<bool> CMAKE_EXPORTCMD = ArgumentStringView{ "--export-commands", "-e" };
    inline Arg<bool> CMAKE_GENSRC = ArgumentStringView{ "--generate-src", "-g" };
    inline Arg<bool> CMAKE_SHOW = ArgumentStringView{ "--show", "-s" };
} // namespace Args
} // namespace ft