#include "uuid.hpp"


namespace sole {


    void to_json(nlohmann::json &j, const sole::uuid &uuid) {
        j = uuid.str();
    }

    void from_json(const nlohmann::json &j, sole::uuid &uuid) {
        uuid = sole::rebuild(j.get<std::string>());
    }

} //ns sole
