#include "common/pack.hpp"

namespace gnt {

    void pack_int(uint64_t i, nonstd::span<char> target) {
        target[0] = (i >> 56) & 0xFF;
        target[1] = (i >> 48) & 0xFF;
        target[2] = (i >> 40) & 0xFF;
        target[3] = (i >> 32) & 0xFF;
        target[4] = (i >> 24) & 0xFF;
        target[5] = (i >> 16) & 0xFF;
        target[6] = (i >> 8) & 0xFF;
        target[7] = (i >> 0) & 0xFF;
    }

    uint64_t unpack_int(nonstd::span<char> chars) {
        if(chars.size() < 8) {
            throw std::logic_error("Unable to unpack int: raw key overflow (likely corrupted).");
        }
        uint64_t up = ((uint64_t) chars[7]) & 0x00000000000000FF;
        up |= (((uint64_t) chars[6]) << 8) &  0x000000000000FF00;
        up |= (((uint64_t) chars[5]) << 16) & 0x0000000000FF0000;
        up |= (((uint64_t) chars[4]) << 24) & 0x00000000FF000000;
        up |= (((uint64_t) chars[3]) << 32) & 0x000000FF00000000;
        up |= (((uint64_t) chars[2]) << 40) & 0x0000FF0000000000;
        up |= (((uint64_t) chars[1]) << 48) & 0x00FF000000000000;
        up |= (((uint64_t) chars[0]) << 56) & 0xFF00000000000000;
        return up;
    }

    uint64_t unpack_int(const std::string &str, const std::size_t offset) {
        if(str.size() < offset + 8) {
            throw std::logic_error("Unable to unpack int: raw key overflow (likely corrupted).");
        }
        return unpack_int(nonstd::span<char>((char *)str.data()+offset, 8));
    }

    void pack_double(double d, nonstd::span<char> chars) {
        // We implment this by creating two 64 bit ints for the int and fractional part of the double, inverting negative
        // ... values, and adding a sign prefix char.
        if(chars.size() < 1 + 8 + 8) {
            throw std::logic_error("Need 1 + 8 + 8 chars to pack double into span.");
        }
        double int_part = 0;
        double frac_part = std::modf(d, &int_part);
        bool is_negative = (d < 0);
        uint64_t int_part_casted = static_cast<uint64_t>(std::abs(int_part));
        if(is_negative) {
            chars[0] = '0';
            int_part_casted = std::numeric_limits<uint64_t>::max() - int_part_casted;
            frac_part = 1.0 - std::abs(frac_part);
        } else {
            chars[0] = '1';
        }
        pack_int(int_part_casted, chars.subspan(1, 8));
        pack_int(frac_part * std::pow(10.0, 15), chars.subspan(1+8, 8));
    }

    double unpack_double(nonstd::span<char> chars) {
        if(chars.size() < 1 + 8 + 8) {
            throw std::logic_error("String view too small to unpack double.");
        }
        auto int_part = unpack_int(chars.subspan(1, 8));
        auto frac_part = unpack_int(chars.subspan(1+8, 8));
        // Adjust back down
        double frac_part_dbl = static_cast<double>(frac_part) / std::pow(10.0, 15);
        if(chars[0] == '0') {
            int_part = std::numeric_limits<uint64_t>::max() - int_part;
            frac_part_dbl = 1.0 - frac_part_dbl;
            return -(static_cast<double>(int_part) + frac_part_dbl);
        }
        return static_cast<double>(int_part) + frac_part_dbl;
    }

    sole::uuid unpack_uuid(nonstd::span<char> chars) {
        auto ab = unpack_int(chars.subspan(0, 8));
        auto cd = unpack_int(chars.subspan(8, 8));
        return sole::uuid{ab, cd};
    }

    void pack_uuid(const sole::uuid &u, nonstd::span<char> sv) {
        if(sv.size() < 8 + 8) {
            throw std::logic_error("Can't pack uuid into string view, string view is too small");
        }
        pack_int(u.ab, sv.subspan(0, 8));
        pack_int(u.cd, sv.subspan(8, 8));
    }
} //ns gnt