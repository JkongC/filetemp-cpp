#pragma once
#include <args.hpp>

#include "log.hpp"
#include "file_io.hpp"

template <typename T>
inline bool check_file_op_result(ft::FileOpResult<T> &result)
{
    if (!result)
    {
#ifdef FT_DEBUG
        ft::log_err(std::visit([](auto &&err) { return err.loc; }, result.error()),
                    "Failed to create file: {}",
                    std::visit([](auto &&err) { return err.msg(); }, result.error()));
#else
        ft::log_err("Failed to create file: {}", std::visit([](auto &&err) { return err.msg(); }, result.error()));
#endif

        return false;
    }

    return true;
}

namespace ft
{
bool gen_cmake_file(argparse::ArgumentParser &args);
} // namespace ft