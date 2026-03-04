#include <iostream>
#include "include/minidb/record.hpp"

int main() {
    minidb::entry r{1704067200, 1};
    
    auto bytes = r.serialize();
    std::cout << "Serializado OK, bytes: ";
    for (auto b : bytes)
        std::cout << (int)b << " ";
    std::cout << std::endl;

    auto result = minidb::entry::deserialize(bytes);
    if (result)
        std::cout << "Deserializado OK, timestamp: " << result->timestamp << std::endl;
    else
        std::cout << "CRC error!" << std::endl;

    return 0;
}