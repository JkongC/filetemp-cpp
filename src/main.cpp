#include <print>
#include <argparse/argparse.hpp>

using namespace argparse;

int main(int argc, char **argv)
{
    // Test code for argparse
    ArgumentParser arg_parser{ "filetemp", "0.1.0" };
    arg_parser.add_argument("verbose").scan<'i', int>();
    arg_parser.add_argument("--content", "-c").metavar("ctn").help("The content you want to print");

    try
    {
        arg_parser.parse_args(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::println(stderr, "{}", e.what());
        return 0;
    }

    std::cout << arg_parser << "\n";
}