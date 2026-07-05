#pragma once
#include <string>
#include <vector>
#include <optional>

namespace domain {

struct Person {
    std::string id;
    std::string nickname;
    std::string name;
    std::string birth_date;
    std::optional<std::vector<std::string>> stack;
};

} // namespace domain
