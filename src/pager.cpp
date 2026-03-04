#include "minidb/pager.hpp"
#include <stdexcept>

namespace minidb {

Pager::Pager(const std::filesystem::path& path) : path_(path) {
    file_.open(path_, std::ios::binary | std::ios::app | std::ios::in);
    if (!file_.is_open())
        throw std::runtime_error("Pager: could not open file: " + path_.string());
}

Pager::~Pager() {
    if (file_.is_open())
        file_.close();
}

bool Pager::write(std::span<const uint8_t> data) {
    file_.seekp(0, std::ios::end);
    file_.write(reinterpret_cast<const char*>(data.data()),
                static_cast<std::streamsize>(data.size()));
    file_.flush();
    return file_.good();
}

std::vector<uint8_t> Pager::read_all() const {
    std::ifstream reader(path_, std::ios::binary);
    if (!reader.is_open())
        return {};

    reader.seekg(0, std::ios::end);
    const std::size_t size = static_cast<std::size_t>(reader.tellg());
    reader.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    reader.read(reinterpret_cast<char*>(buffer.data()),
                static_cast<std::streamsize>(size));
    return buffer;
}

std::size_t Pager::entry_count() {
    file_.seekg(0, std::ios::end);
    const std::size_t size = static_cast<std::size_t>(file_.tellg());
    return size / Entry::SIZE;
}

} // namespace minidb