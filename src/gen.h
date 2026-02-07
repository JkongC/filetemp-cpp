#pragma once

#include <memory>
#include <type_traits>

#include "argparse/argparse.hpp"
#include "file_types.h"

namespace ft
{
namespace detail
{
    class OutputBase
    {
    public:
        virtual bool do_output() = 0;

        OutputBase(const OutputBase &) = delete;
        OutputBase &operator=(const OutputBase &) = delete;
        OutputBase(OutputBase &&) = delete;
        OutputBase &operator=(OutputBase &&) = delete;

        virtual ~OutputBase() = default;

    protected:
        OutputBase() noexcept = default;
    };

    template <typename T>
    concept ImplOutput = requires(T t) {
        requires std::is_nothrow_constructible_v<T>;
        t.output();
    };

    template <ImplOutput T>
    class OutputAdapter final : public OutputBase
    {
    public:
        OutputAdapter() noexcept {}

        OutputAdapter(const OutputAdapter &) = delete;
        OutputAdapter &operator=(const OutputAdapter &) = delete;
        OutputAdapter(OutputAdapter &&) = delete;
        OutputAdapter &operator=(OutputAdapter &&) = delete;

        virtual bool do_output() { return obj.output(); }

        virtual ~OutputAdapter() = default;

    private:
        T obj;
    };

    class CacherBase
    {
    public:
        CacherBase(const CacherBase &) = delete;
        CacherBase &operator=(const CacherBase &) = delete;
        CacherBase(CacherBase &&) = delete;
        CacherBase &operator=(CacherBase &&) = delete;

        virtual ~CacherBase() = default;

    protected:
        CacherBase() noexcept = default;
    };

    template <typename T>
    concept ImplCacher = requires(T t) {
        requires std::is_constructible_v<T, argparse::ArgumentParser &>;
        t.update();
    };

    template <ImplCacher T>
    class CacherAdapter final : public CacherBase
    {
    public:
        CacherAdapter(argparse::ArgumentParser &parser) noexcept
            : obj(parser)
        {
        }

        CacherAdapter(const CacherAdapter &) = delete;
        CacherAdapter &operator=(const CacherAdapter &) = delete;
        CacherAdapter(CacherAdapter &&) = delete;
        CacherAdapter &operator=(CacherAdapter &&) = delete;

        virtual ~CacherAdapter() override { obj.update(); }

    private:
        T obj;
    };
} // namespace detail

class Output
{
public:
    static Output create(FileType type);

    Output(Output &&) = default;
    Output &operator=(Output &&) = default;

    bool output();

private:
    Output(std::unique_ptr<detail::OutputBase> base)
        : m_base(std::move(base))
    {
    }

private:
    std::unique_ptr<detail::OutputBase> m_base;
};

class ScopeCacher
{
public:
    // Ctor optionally loads caches from file to ArgumentStorage
    [[nodiscard("ScopeCacher's correctness relies on its lifetime")]]
    static ScopeCacher create(FileType type, argparse::ArgumentParser &parser);

    ScopeCacher(ScopeCacher &&) = default;
    ScopeCacher &operator=(ScopeCacher &&) = default;

private:
    ScopeCacher(std::unique_ptr<detail::CacherBase> base)
        : m_base(std::move(base))
    {
    }

private:
    std::unique_ptr<detail::CacherBase> m_base;
};

} // namespace ft