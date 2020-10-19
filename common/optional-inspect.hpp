#pragma once

#include <optional>
#include <type_traits>


namespace std {

    template <class Inspector, typename T>
    typename Inspector::result_type inspect(Inspector& f, optional<T>& x) {
        T dflt;
        if(x) {
            dflt = x.value();
        }
        bool b = x;
        if constexpr(!std::is_same_v<typename Inspector::result_type, void>) {
            auto res = f(caf::meta::type_name("std::optional<T>"), b, dflt);
            if(b) {
                x.emplace(dflt);
            }
            return res;
        } else {
            f(caf::meta::type_name("std::optional<T>"), b, dflt);
            if(b) {
                x.emplace(dflt);
            }
        }
    }

}