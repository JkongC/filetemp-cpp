#pragma once

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <cstdio>
#include <memory>
#include <optional>
#include <concepts>
#include <system_error>
#include <vector>
#include <span>

namespace ft
{

template <typename T>
concept Serializable = requires(std::remove_cvref_t<T> obj, std::span<std::byte, 1> v) {
    { obj.serialize() } -> std::convertible_to<std::vector<std::byte>>;
    obj.deserialize(v);
};

template <typename T>
concept GeneralSerializable = Serializable<T> || std::is_trivially_copyable_v<std::remove_cvref_t<T>>;

class File
{
public:
    enum class Mode
    {
        read,
        write
    };

public:
    static std::optional<File> create(const std::filesystem::path &file_path, Mode mode, bool pre_fetch = false)
    {
        File ret{ file_path, mode, pre_fetch };
        if (ret.valid())
        {
            return std::move(ret);
        }
        else
        {
            return std::nullopt;
        }
    }

public:
    File(File &&another)
        : m_path(std::move(another.m_path))
    {
        m_handle.swap(another.m_handle);
    }
    File(const File &) = delete;

    File &operator=(const File &) = delete;
    File &operator=(this File &self, File &&another)
    {
        self.m_path = std::move(another.m_path);
        self.m_handle.swap(another.m_handle);
        return self;
    }

    bool valid(this const File &self) { return self.m_valid; }
    const auto &get_path(this const File &self) { return self.m_path; }

    template <GeneralSerializable T>
    bool write_into(this File &self, T &obj)
    {
        if constexpr (Serializable<T>)
        {
            std::vector<std::byte> buf = obj.serialize();
            auto written_count = std::fwrite(buf.data(), buf.size() * sizeof(std::byte), 1, self.m_handle.get());
            return written_count == 1;
        }
        else
        {
            auto written_count = std::fwrite(&obj, sizeof(T), 1, self.m_handle.get());
            return written_count == 1;
        }
    }

    template <GeneralSerializable T>
    bool read_from(this File &self, T &obj)
    {
        if constexpr (Serializable<T>)
        {
            if (self.m_pre_fetch)
            {
                if (self.m_wholebuf.end() - self.m_wholebuf_it < sizeof(T))
                {
                    return false;
                }
                obj.deserialize(std::span(self.m_wholebuf_it, sizeof(T)));
                self.m_wholebuf_it += sizeof(T);
            }
            else
            {
                std::vector<std::byte> buf(sizeof(obj), std::byte{});
                auto read_count = std::fread(buf.data(), sizeof(obj), 1, self.m_handle.get());
                if (read_count != 1)
                {
                    return false;
                }
                obj.deserialize(std::span(buf.begin(), buf.end()));
            }
        }
        else
        {
            if (self.m_pre_fetch)
            {
                if (self.m_wholebuf.end() - self.m_wholebuf_it < sizeof(T))
                {
                    return false;
                }
                std::memcpy(&obj, &*self.m_wholebuf_it, sizeof(T));
                self.m_wholebuf_it += sizeof(T);
            }
            else
            {
                auto read_count = std::fread(&obj, sizeof(obj), 1, self.m_handle.get());
                if (read_count != 1)
                {
                    return false;
                }
            }
        }

        return true;
    }

private:
    File(const std::filesystem::path &file_path, Mode mode, bool pre_fetch)
        : m_path(file_path)
        , m_pre_fetch(pre_fetch)
    {
        std::error_code ec;
        auto size = std::filesystem::file_size(file_path, ec);
        if (ec)
        {
            return;
        }

        auto ptr = std::fopen(reinterpret_cast<const char *>(file_path.u8string().c_str()), mode_str(mode));
        m_handle.reset(ptr);

        if (mode == Mode::read && pre_fetch && m_handle)
        {
            m_wholebuf.resize(size);
            m_wholebuf_it = m_wholebuf.begin();
            auto read_count = std::fread(m_wholebuf.data(), size, 1, m_handle.get());
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

    static const char *mode_str(Mode mode)
    {
        switch (mode)
        {
        case Mode::read:
            return "rb+";
            break;
        case Mode::write:
            return "wb+";
            break;
        }
    }

private:
    std::vector<std::byte> m_wholebuf;
    std::filesystem::path m_path;
    std::unique_ptr<std::FILE, void (*)(std::FILE *)> m_handle{ nullptr, File::release_file_handle };
    std::vector<std::byte>::iterator m_wholebuf_it;
    bool m_pre_fetch;
    bool m_valid = false;
};
} // namespace ft