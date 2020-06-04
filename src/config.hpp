#pragma once

#include <tuple>
#include <string>
#include <stdexcept>

#include "caf/actor_system_config.hpp"

#include "common/logger.hpp"

namespace as {

    using config_group = caf::actor_system_config::opt_group;

    //Config that calls a static method on Configurables to add custom option groups
    template<typename...Configurables>
    class config : public caf::actor_system_config {
    public:

        config() {
            // Change some defaults to be more appropriate
            const std::string dflt_log = "%d [%t %a] %p %c (%M) %F:%L %m%n";
            const std::string dflt_colour = "coloured";
            const std::string dflt_verbosity = "info";
            set("logger.file-format", dflt_log);
            set("logger.console-format", dflt_log);
            set("logger.console", dflt_colour);
            set("logger.verbosity", dflt_verbosity);
            if constexpr(sizeof...(Configurables) > 0) {
                apply_custom_options<Configurables...>(caf::actor_system_config::custom_options_);
            }
        }

        //! Get the requisite configurable config struct.
        template<typename Config>
        Config &get() {
            return std::get<Config>(_config_variants);
        }

        caf::error parse(int argc, char** argv,
                                   const char* ini_file_cstr = nullptr) {
            auto err = caf::actor_system_config::parse(argc, argv, ini_file_cstr);
            if(err) {
                return err;
            }
            // Inject the same verbosity into our own logger.
            common::logger::set_verbosity(caf::get<std::string>(*this, "logger.verbosity"));
            return {};
        }

        std::size_t positional_arg_count() const {
            return caf::actor_system_config::remainder.size();
        }

        const std::string &positional_arg(std::size_t idx) const {
            if(idx >= positional_arg_count()) {
                throw std::logic_error("Config positional arg out of bounds.");
            }
            return caf::actor_system_config::remainder.at(idx);
        }

    private:

        //Just apply to each one
        template<typename HeadConfig, typename NextConfig, typename... TailConfigs>
        void apply_custom_options(caf::config_option_set &custom_options) {
            apply_custom_options<HeadConfig>(custom_options);
            apply_custom_options<NextConfig, TailConfigs...>(custom_options);
        }

        template<typename Config>
        void apply_custom_options(caf::config_option_set &custom_options) {
            Config &option_struct = std::get<Config>(_config_variants);
            config_group option_group = opt_group(custom_options, option_struct.group_name());
            option_struct.configure(option_group);
        }

        std::tuple<Configurables...> _config_variants;

    };

} //ns as
