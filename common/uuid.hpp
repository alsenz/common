#pragma once

#include <vector>
#include <cstddef>

#include "caf/all.hpp"

#include "nlohmann/json.hpp"

#include "sole.hpp"

namespace sole {

    // CAF inspection override!
    template<class Inspector>
    typename Inspector::result_type inspect(Inspector &f, sole::uuid &x) {
        return f(caf::meta::type_name("sole::uuid"), x.ab, x.cd);
    }

    void to_json(nlohmann::json &j, const sole::uuid &uuid);

    void from_json(const nlohmann::json &j, sole::uuid &uuid);

} //ns sole
