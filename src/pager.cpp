#include "minidb/pager.hpp"
#include "minidb/entry.hpp"

namespace minidb {

minidb::Pager::Pager(const std::filesystem::path& path) : path_(path) {
    // Open the file in binary mode for both reading and appending. Create it if it doesn't exist.
    file_.open(path_, std::ios::binary | std::ios::app | std::ios::in);
    if (!file_.is_open())
        throw std::runtime_error("Pager: could not open file: " + path_.string());
}

Pager::~Pager() { // Ensure the file is closed when the Pager is destroyed.
    if (file_.is_open())
        file_.close();
}

bool Pager::write(std::span<const uint8_t> data) {
    file_.seekp(0, std::ios::end); // Move the write pointer to the end of the file for appending
    // Write the data to the file
    file_.write(reinterpret_cast<const char*>(data.data()),
                static_cast<std::streamsize>(data.size()));
    file_.flush(); // Ensure data is written to disk
    return file_.good();
}

std::vector<uint8_t> Pager::read_all() const {
    std::ifstream reader(path_, std::ios::binary); // Open a separate ifstream for reading to avoid interfering with the write stream
    if (!reader.is_open())
        return {};
    // Read the entire file into a buffer
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
    return size / minidb::Entry::SIZE;
}

} // namespace minidb