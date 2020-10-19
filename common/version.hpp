#pragma once

#include <optional>

#include "nlohmann/json.hpp"

namespace as::common {

    struct version {
        unsigned int major;
        unsigned int minor;
        std::optional<unsigned int> build;
    };

    void to_json(nlohmann::json &j, const version &v);

} //ns as::common
