#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace easykey
{

    class Database
    {
        private:
            std::unordered_map<std::string, std::vector<std::uint8_t>> content;
        public:
            void write(const std::string, const std::vector<std::uint8_t>);
            std::vector<std::uint8_t> read(const std::string key);
    }; 

};