#include <argparse/argparse.hpp>
#include <exception>

#include "args.hpp"
#include "gen.h"

using namespace argparse;
using namespace ft;

#define ARG(def) def.name.full(), def.name.short_name()
#define ARGT(a) ArgRepr<arg_ident(a)>

int main(int argc, char **argv)
{
    ArgumentStorage args;

    ArgumentParser program{ "filetemp", "0.1.0" };

    ArgumentParser cmake_parser{ "cmake", "", default_arguments::help };
    cmake_parser.add_argument(Arg::CMAKE_WORKDIRECTORY)
        .help("The output directory")
        .default_value<std::string>(".")
        .store_into(args.get(Arg::CMAKE_WORKDIRECTORY));
    cmake_parser.add_argument(ARG(Arg::CMAKE_VERSION))
        .help("Minimum cmake version")
        .default_value<ArgRepr<Arg::CMAKE_VERSION.ident>>("3.0")
        .metavar("<ver>")
        .store_into(args.get(Arg::CMAKE_VERSION));
    cmake_parser.add_argument(ARG(Arg::CMAKE_CSTD))
        .help("C standard")
        .scan<'i', ArgRepr<Arg::CMAKE_CSTD.ident>>()
        .default_value(99)
        .metavar("<std>")
        .store_into(args.get<ArgRepr<Arg::CMAKE_CSTD.ident>>(Arg::CMAKE_CSTD));
    cmake_parser.add_argument(ARG(Arg::CMAKE_CXXSTD))
        .help("C++ standard")
        .scan<'i', ArgRepr<Arg::CMAKE_CXXSTD.ident>>()
        .default_value(20)
        .metavar("<std>")
        .store_into(args.get<ArgRepr<Arg::CMAKE_CSTD.ident>>(Arg::CMAKE_CXXSTD));
    cmake_parser.add_argument(ARG(Arg::CMAKE_PROJECT))
        .help("Project and executable name")
        .default_value<std::string>("foo")
        .metavar("<name>")
        .store_into(args.get(Arg::CMAKE_PROJECT));
    cmake_parser.add_argument(ARG(Arg::CMAKE_MAINLANG))
        .help("Main language of the project")
        .default_value<std::string>("CXX")
        .metavar("<lang>")
        .store_into(args.get(Arg::CMAKE_MAINLANG));
    cmake_parser.add_argument(ARG(Arg::CMAKE_SAVEAS))
        .help("Save current options to config cache")
        .metavar("<config_name>")
        .store_into(args.get(Arg::CMAKE_SAVEAS));
    cmake_parser.add_argument(ARG(Arg::CMAKE_USECONFIG))
        .help("Use config cache")
        .metavar("<config_name>")
        .store_into(args.get(Arg::CMAKE_USECONFIG));
    cmake_parser.add_argument(ARG(Arg::CMAKE_EXPORTCMD))
        .help("Export compile commands")
        .flag()
        .store_into(args.get<ArgRepr<Arg::CMAKE_EXPORTCMD.ident>>(Arg::CMAKE_EXPORTCMD));
    cmake_parser.add_argument(ARG(Arg::CMAKE_GENSRC))
        .help("Generate source file")
        .flag()
        .store_into(args.get<ArgRepr<Arg::CMAKE_GENSRC.ident>>(Arg::CMAKE_GENSRC));
    cmake_parser.add_argument(ARG(Arg::CMAKE_SHOW))
        .help("Show output to console")
        .flag()
        .store_into(args.get<ArgRepr<Arg::CMAKE_SHOW.ident>>(Arg::CMAKE_SHOW));

    program.add_subparser(cmake_parser);

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return -1;
    }

    if (argc < 2)
    {
        std::cout << program;
    }

    auto run_output = [&](FileType type)
    {
        auto cacher = ScopeCacher::create(type, cmake_parser, args);
        auto gen = Output::create(type, args);
        return gen.output();
    };

    if (program.is_subcommand_used("cmake"))
    {
        if (!run_output(FileType::CMake))
        {
            return -1;
        }
    }
}