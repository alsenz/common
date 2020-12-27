#pragma once

#include "caf/all.hpp"

#include <cstddef>

namespace gnt::grain {

    namespace detail {

        int constexpr cstrlen(const char *str) {
            return *str ? 1 + cstrlen(str + 1) : 0;
        }

        constexpr uint32_t hash_32_unroll(const char *str, uint32_t h) {
            if (cstrlen(str) > 0) {
                h += *str;
                h += (h << 10);
                h ^= (h >> 6);
                h = hash_32_unroll(str + 1, h);
            }
            return h;
        }

    }

    constexpr uint32_t hash_32(const char *str) {
        uint32_t h = detail::hash_32_unroll(str, 7);
        h += (h << 3);
        h ^= (h >> 11);
        h += (h << 15);
        return h;
    }

    constexpr uint16_t hash_16(const char *str) {
        return static_cast<uint16_t>(hash_32(str) & 0xFFFF);
    }

    template<typename T>
    void init_caf_type() {
        caf::detail::meta_object src[] = {
            caf::detail::make_meta_object<T>(caf::type_name_v<T>)
        };
        caf::detail::set_global_meta_objects(caf::type_id_v<T>, caf::make_span(src));
    }

} //ns gnt::grain


#define ENABLE_CAF_TYPE(message_type)                                          \
    namespace caf {                                                            \
    template <>                                                                \
    struct type_id<message_type> {                                             \
      static constexpr type_id_t value                                         \
         = gnt::grain::hash_16(CAF_PP_STR(message_type));                      \
    };                                                                         \
    template <>                                                                \
    struct type_by_id<type_id<message_type>::value> {                          \
      using type = message_type;                                               \
    };                                                                         \
    template <>                                                                \
    struct type_name<message_type> {                                           \
      static constexpr const char* value                                       \
        = CAF_PP_STR(message_type);                                            \
    };                                                                         \
    template <>                                                                \
    struct type_name_by_id<type_id<message_type>::value> \
      : type_name<message_type> {};                      \
    }



struct test {};

ENABLE_CAF_TYPE(test)