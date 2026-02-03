#include <argparse/argparse.hpp>
#include <exception>

#include "cmake_gen.h"

using namespace argparse;

int main(int argc, char **argv)
{
    ArgumentParser program{ "filetemp", "0.1.0" };

    ArgumentParser cmake_parser{ "cmake", "", default_arguments::help };
    cmake_parser.add_argument("directory").help("The output directory").default_value<std::string>(".");
    cmake_parser.add_argument("--version", "-v").help("Minimum cmake version").default_value<std::string>("3.0");
    cmake_parser.add_argument("--cstd", "-c").help("C standard").default_value<int>(99).choices("99", "11", "17", "23");
    cmake_parser.add_argument("--cxxstd", "-C")
        .help("C++ standard")
        .default_value<int>(20)
        .choices("99", "11", "14", "17", "20", "23", "26");
    cmake_parser.add_argument("--project", "-p").help("Project and executable name").default_value<std::string>("foo");
    cmake_parser.add_argument("--main-lang", "-m")
        .help("Main language of the project")
        .default_value("CXX")
        .choices("CXX", "C");
    cmake_parser.add_argument("--export-commands", "-e").help("Export compile commands").flag();
    cmake_parser.add_argument("--generate-src", "-g").help("Generate source file").flag();
    cmake_parser.add_argument("--show", "-s").help("Show output to console").flag();

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

    if (program.is_subcommand_used("cmake"))
    {
        if (!ft::gen_cmake_file(cmake_parser))
        {
            return -1;
        }
    }
}