#include "common/version.hpp"

namespace as::common {

    void to_json(nlohmann::json &j, const version &v) {
        j = nlohmann::json{{"major", v.major}, {"minor", v.minor}};
        if(v.build) {
            j["build"] = v.build.value();
        }
    }

} //ns as::common
