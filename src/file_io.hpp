#pragma once

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <cstdio>
#include <format>
#include <memory>
#include <expected>
#include <concepts>
#include <ostream>
#include <system_error>
#include <variant>
#include <vector>
#include <string>
#include <span>

#ifdef FT_DEBUG
#include <source_location>
#endif

namespace ft
{

template <typename T>
concept ManSerializable = requires(std::remove_cvref_t<T> obj, std::span<std::byte> v) {
    { obj.serialize() } -> std::convertible_to<std::vector<std::byte>>;
    obj.deserialize(v);
};

template <typename T>
concept Printable = std::convertible_to<std::remove_cvref_t<T>, std::string_view>;

template <typename T>
concept Serializable = ManSerializable<T> || std::is_trivially_copyable_v<std::remove_cvref_t<T>>;

template <typename T>
concept GeneralSerializable = Serializable<T> || Printable<T>;

namespace detail
{
    template <typename T>
    concept FileOpErrReq = requires(T err) {
        { err.msg() } -> std::convertible_to<std::string>;
    };

    template <typename... Ts>
        requires(FileOpErrReq<Ts> && ...)
    using FileOpErrVar = std::variant<Ts...>;

    struct FileOpErrBase
    {
        std::filesystem::path file;
#ifdef FT_DEBUG
        std::source_location loc = std::source_location::current();
#endif

        FileOpErrBase(const std::filesystem::path &f)
            : file(f)
        {
        }
    };

    template <typename T>
    struct BasicFileOpErr : FileOpErrBase
    {
        std::string msg(this T &self) { return std::format(T::fmt_msg, self.file.string()); }
    };

} // namespace detail

enum class FileMode
{
    read,
    write
};

inline constexpr std::string_view stringify_filemode(FileMode mode)
{
    switch (mode)
    {
    case FileMode::read:
        return "read";
        break;
    case FileMode::write:
        return "write";
        break;
    default:
        return "";
        break;
    }
}

struct FileOpenFailed : detail::BasicFileOpErr<FileOpenFailed>
{
    static constexpr std::string_view fmt_msg = R"("{}": Failed to open file.)";
};

struct FileWriteFailed : detail::BasicFileOpErr<FileWriteFailed>
{
    static constexpr std::string_view fmt_msg = R"("{}": Failed to write into file.)";
};

struct FileReadFailed : detail::BasicFileOpErr<FileReadFailed>
{
    static constexpr std::string_view fmt_msg = R"("{}": Failed to read from file.)";
};

struct ModeInconsistent : detail::FileOpErrBase
{
    FileMode mode_active;
    FileMode mode_requested;

    std::string msg(this const ModeInconsistent &self)
    {
        return std::format(R"("{}": Operation failed requesting mode "{}", while the active mode is "{}")",
                           self.file.string(),
                           stringify_filemode(self.mode_requested),
                           stringify_filemode(self.mode_active));
    }
};

using FileOpErr = detail::FileOpErrVar<FileOpenFailed, FileWriteFailed, FileReadFailed, ModeInconsistent>;

template <typename T = void>
using FileOpResult = std::expected<T, FileOpErr>;

class File
{
public:
    static FileOpResult<File> create(const std::filesystem::path &file_path, FileMode mode, bool use_buffer = false)
    {
        File ret{ file_path, mode, use_buffer };
        if (ret.valid())
        {
            return std::move(ret);
        }
        else
        {
            return std::unexpected{ FileOpenFailed{ file_path } };
        }
    }

public:
    File(File &&another)
        : m_path(std::move(another.m_path))
        , m_buf(std::move(another.m_buf))
        , m_buf_it(another.m_buf_it)
        , m_mode(another.m_mode)
        , m_valid(another.m_valid)
        , m_use_buffer(another.m_use_buffer)
    {
        m_handle.swap(another.m_handle);
    }
    File(const File &) = delete;

    File &operator=(const File &) = delete;
    File &operator=(this File &self, File &&another)
    {
        self.m_path = std::move(another.m_path);
        self.m_handle.swap(another.m_handle);
        self.m_buf = std::move(another.m_buf);
        self.m_buf_it = another.m_buf_it;
        self.m_mode = another.m_mode;
        self.m_valid = another.m_valid;
        self.m_use_buffer = another.m_use_buffer;
        return self;
    }

    ~File()
    {
        if (m_use_buffer && m_mode == FileMode::write)
        {
            std::ignore = flush();
        }
    }

    bool valid(this const File &self) { return self.m_valid; }
    operator bool(this File &self) { return self.valid(); }

    const auto &get_path(this const File &self) { return self.m_path; }
    FileMode get_mode(this const File &self) { return self.m_mode; }

    template <GeneralSerializable T>
    FileOpResult<> write(this File &self, const T &obj)
    {
        if (self.get_mode() != FileMode::write)
        {
            return std::unexpected{ ModeInconsistent{ self.m_path, self.m_mode, FileMode::write } };
        }

        if constexpr (ManSerializable<T>)
        {
            std::vector<std::byte> buf = obj.serialize();
            if (self.m_use_buffer)
            {
                self.m_buf.append_range(buf);
            }
            else
            {
                auto written_count = std::fwrite(buf.data(), buf.size() * sizeof(std::byte), 1, self.m_handle.get());
                if (written_count != 1)
                {
                    return std::unexpected{ FileWriteFailed{ self.m_path } };
                }
            }
        }
        else if constexpr (Printable<T>)
        {
            std::string_view str = obj;
            if (self.m_use_buffer)
            {
                self.m_buf.resize(self.m_buf.size() + str.size());
                std::memcpy(&*(self.m_buf.end() - str.size()), str.data(), str.size());
            }
            else
            {
                auto written_count = std::fwrite(str.data(), str.size() * sizeof(char), 1, self.m_handle.get());
                if (written_count != 1)
                {
                    return std::unexpected{ FileWriteFailed{ self.m_path } };
                }
            }
        }
        else
        {
            if (self.m_use_buffer)
            {
                self.m_buf.resize(self.m_buf.size() + sizeof(T));
                std::memcpy(&*(self.m_buf.end() - sizeof(T)), &obj, sizeof(T));
            }
            else
            {
                auto written_count = std::fwrite(&obj, sizeof(T), 1, self.m_handle.get());
                if (written_count != 1)
                {
                    return std::unexpected{ FileWriteFailed{ self.m_path } };
                }
            }
        }

        return {};
    }

    template <GeneralSerializable... Ts>
    FileOpResult<> batch_write(this File &self, const Ts &...objs)
    {
        FileOpResult<> ret;
        if (((ret = self.write(objs)) && ...))
        {
            return {};
        }
        return ret;
    }

    template <Serializable T>
    FileOpResult<> read(this File &self, T &obj)
    {
        if (self.m_mode == FileMode::read)
        {
            return std::unexpected{ ModeInconsistent{ self.m_path, self.m_mode, FileMode::read } };
        }

        if constexpr (Serializable<T>)
        {
            if (self.m_use_buffer)
            {
                if (self.m_buf.end() - self.m_buf_it < sizeof(T))
                {
                    return std::unexpected{ FileReadFailed{ self.m_path } };
                }
                obj.deserialize(std::span(self.m_buf_it, sizeof(T)));
                self.m_buf_it += sizeof(T);
            }
            else
            {
                std::vector<std::byte> buf(sizeof(obj), std::byte{});
                auto read_count = std::fread(buf.data(), sizeof(obj), 1, self.m_handle.get());
                if (read_count != 1)
                {
                    return std::unexpected{ FileReadFailed{ self.m_path } };
                }
                obj.deserialize(std::span(buf.begin(), buf.end()));
            }
        }
        else
        {
            if (self.m_use_buffer)
            {
                if (self.m_buf.end() - self.m_buf_it < sizeof(T))
                {
                    return std::unexpected{ FileReadFailed{ self.m_path } };
                }
                std::memcpy(&obj, &*self.m_buf_it, sizeof(T));
                self.m_buf_it += sizeof(T);
            }
            else
            {
                auto read_count = std::fread(&obj, sizeof(obj), 1, self.m_handle.get());
                if (read_count != 1)
                {
                    return std::unexpected{ FileReadFailed{ self.m_path } };
                }
            }
        }

        return {};
    }

    FileOpResult<> read(this File &self, std::span<std::byte> buf)
    {
        if (self.m_mode == FileMode::write)
        {
            return std::unexpected{ ModeInconsistent{ self.m_path, self.m_mode, FileMode::read } };
        }

        if (self.m_use_buffer)
        {
            if (self.m_buf.end() - self.m_buf_it < buf.size())
            {
                return std::unexpected{ FileReadFailed{ self.m_path } };
            }
            std::memcpy(buf.data(), &*self.m_buf_it, buf.size());
            self.m_buf_it += buf.size();
        }
        else
        {
            auto read_size = std::fread(buf.data(), buf.size(), 1, self.m_handle.get());
            if (read_size != 1)
            {
                return std::unexpected{ FileReadFailed{ self.m_path } };
            }
        }

        return {};
    }

    FileOpResult<> padding(this File &self, std::size_t count, std::byte byte = std::byte{ 0 })
    {
        static std::vector<std::byte> temp_buf{};

        if (self.m_mode == FileMode::read)
        {
            return std::unexpected{ ModeInconsistent{ self.m_path, self.m_mode, FileMode::write } };
        }

        if (self.m_use_buffer)
        {
            self.m_buf.resize(self.m_buf.size() + count, byte);
            return {};
        }

        if (temp_buf.size() < count)
        {
            temp_buf.resize(count, byte);
        }
        else
        {
            std::memset(temp_buf.data(), static_cast<int>(byte), count);
        }

        auto written_count = std::fwrite(temp_buf.data(), count * sizeof(std::byte), 1, self.m_handle.get());
        if (written_count != 1)
        {
            return std::unexpected{ FileReadFailed{ self.m_path } };
        }

        return {};
    }

    FileOpResult<> flush(this File &self)
    {
        if (self.m_mode == FileMode::read)
        {
            return std::unexpected{ ModeInconsistent{ self.m_path, self.m_mode, FileMode::write } };
        }

        if (!self.m_use_buffer)
        {
            return {};
        }

        auto written_count = std::fwrite(self.m_buf.data(), self.m_buf.size(), 1, self.m_handle.get());
        if (written_count != 1)
        {
            return std::unexpected{ FileWriteFailed{ self.m_path } };
        }

        self.m_buf.clear();
        self.m_buf_it = self.m_buf.begin();

        return {};
    }

    FileOpResult<> flush_to(this File &self, std::ostream &os)
    {
        if (self.m_mode == FileMode::read)
        {
            return std::unexpected{ ModeInconsistent{ self.m_path, self.m_mode, FileMode::write } };
        }

        if (self.m_use_buffer)
        {
            os.write(reinterpret_cast<const char *>(self.m_buf.data()), self.m_buf.size());
        }
        return {};
    }

private:
    File(const std::filesystem::path &file_path, FileMode mode, bool use_buffer)
        : m_path(file_path)
        , m_use_buffer(use_buffer)
        , m_mode(mode)
    {
        std::error_code ec;
        auto size = std::filesystem::file_size(file_path, ec);
        if (mode == FileMode::read && ec)
        {
            return;
        }

        auto fptr = std::fopen(reinterpret_cast<const char *>(file_path.u8string().c_str()), mode_flag(mode));
        if (!fptr)
        {
            return;
        }
        m_handle.reset(fptr);

        if (mode == FileMode::read && use_buffer && m_handle)
        {
            m_buf.resize(size);
            m_buf_it = m_buf.begin();
            auto read_count = std::fread(m_buf.data(), size, 1, m_handle.get());
            close_file();
            if (read_count != 1)
            {
                return;
            }
        }

        m_valid = true;
    }

    void close_file(this File &self) { self.m_handle.reset(); }

    static void release_file_handle(std::FILE *f) { std::fclose(f); }

    static constexpr const char *mode_flag(FileMode mode)
    {
        switch (mode)
        {
        case FileMode::read:
            return "rb+";
            break;
        case FileMode::write:
            return "wb+";
            break;
        default:
            return "";
            break;
        }
    }

private:
    std::vector<std::byte> m_buf;
    std::filesystem::path m_path;
    std::unique_ptr<std::FILE, void (*)(std::FILE *)> m_handle{ nullptr, File::release_file_handle };
    std::vector<std::byte>::iterator m_buf_it;
    bool m_use_buffer;
    bool m_valid = false;
    FileMode m_mode;
};
} // namespace ft