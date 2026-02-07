#include <argparse/argparse.hpp>
#include <exception>

#include "arg/arg_basic.h"
#include "arg/arg_def.h"
#include "arg/args.h"
#include "gen.h"

using namespace argparse;
using namespace ft;

#define ARG(def) def.full_name(), def.short_name()

int main(int argc, char **argv)
{
    ArgumentParser program{ "filetemp", "0.1.0" };

    ArgumentParser cmake_parser{ "cmake", "", default_arguments::help };
    cmake_parser.add_argument(Args::CMAKE_WORKDIRECTORY.full_name())
        .help("The output directory")
        .default_value<std::string>(".")
        .store_into(&Args::CMAKE_WORKDIRECTORY);
    cmake_parser.add_argument(ARG(Args::CMAKE_VERSION))
        .help("Minimum cmake version")
        .default_value<std::string>("3.0")
        .metavar("<ver>")
        .store_into(&Args::CMAKE_VERSION);
    cmake_parser.add_argument(ARG(Args::CMAKE_CSTD))
        .help("C standard")
        .scan<'i', ArgType(Args::CMAKE_CSTD)>()
        .default_value(99)
        .metavar("<std>")
        .store_into(&Args::CMAKE_CSTD);
    cmake_parser.add_argument(ARG(Args::CMAKE_CXXSTD))
        .help("C++ standard")
        .scan<'i', ArgType(Args::CMAKE_CXXSTD)>()
        .default_value(20)
        .metavar("<std>")
        .store_into(&Args::CMAKE_CXXSTD);
    cmake_parser.add_argument(ARG(Args::CMAKE_PROJECT))
        .help("Project and executable name")
        .default_value<std::string>("foo")
        .metavar("<name>")
        .store_into(&Args::CMAKE_PROJECT);
    cmake_parser.add_argument(ARG(Args::CMAKE_MAINLANG))
        .help("Main language of the project")
        .default_value<std::string>("CXX")
        .metavar("<lang>")
        .store_into(&Args::CMAKE_MAINLANG);
    cmake_parser.add_argument(ARG(Args::CMAKE_SAVEAS))
        .help("Save current options to config cache")
        .metavar("<config_name>")
        .store_into(&Args::CMAKE_SAVEAS);
    cmake_parser.add_argument(ARG(Args::CMAKE_USECONFIG))
        .help("Use config cache")
        .metavar("<config_name>")
        .store_into(&Args::CMAKE_USECONFIG);
    cmake_parser.add_argument(ARG(Args::CMAKE_EXPORTCMD))
        .help("Export compile commands")
        .flag()
        .store_into(&Args::CMAKE_EXPORTCMD);
    cmake_parser.add_argument(ARG(Args::CMAKE_GENSRC))
        .help("Generate source file")
        .flag()
        .store_into(&Args::CMAKE_GENSRC);
    cmake_parser.add_argument(ARG(Args::CMAKE_SHOW))
        .help("Show output to console")
        .flag()
        .store_into(&Args::CMAKE_SHOW);

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
        auto cacher = ScopeCacher::create(type, cmake_parser);
        auto gen = Output::create(type);
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