#pragma once

#include <tuple>
#include <string>
#include <stdexcept>
#include <type_traits>

#include "caf/actor_system_config.hpp"
#include "caf/all.hpp"
#include "caf/io/all.hpp"

#include "common/logger.hpp"
#include "automatic-typeids.hpp"

namespace as {

    using config_group = caf::actor_system_config::opt_group;

    //Configurables is a concept that has .configure(...)
    //and has a tuple<T...> called message_types which need to be inited.

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

        static void init_meta_objects() {
            caf::core::init_global_meta_objects();
            caf::io::middleman::init_global_meta_objects();
            init_meta_objects_impl<Configurables...>();
        }

    private:

        template<typename HeadConfig, typename NextConfig, typename... TailConfigs>
        static void init_meta_objects_impl() {
            init_meta_objects_impl<HeadConfig>();
            init_meta_objects_impl<NextConfig, TailConfigs...>();
        }

        template<typename Config>
        static auto init_meta_objects_impl() -> decltype(typename Config::message_types(), void()) {
            std::cout << "halleee" << std::endl;
            init_meta_objects_impl(typename Config::message_types());
        }

        template<typename Config>
        static void init_meta_objects_impl()  {
            std::cout << "nooo" << std::endl;
            //No-op
        }

        template<typename T, typename Next, typename... Ts>
        static void init_meta_objects_impl(std::tuple<T, Next, Ts...>) {
            init_meta_objects_impl(std::tuple<T>());
            init_meta_objects_impl(std::tuple<Next, Ts...>());
        }

        template<typename T>
        static void init_meta_objects_impl(std::tuple<T>) {
            gnt::grain::init_caf_type<T>();
        }

        //Just apply to each one
        template<typename HeadConfig, typename NextConfig, typename... TailConfigs>
        void apply_custom_options(caf::config_option_set &custom_options) {
            apply_custom_options<HeadConfig>(custom_options);
            apply_custom_options<NextConfig, TailConfigs...>(custom_options);
        }

        template<typename Config>
        void apply_custom_options(caf::config_option_set &custom_options) {
            //TODO only call this if the method exists...
            //TODO TODO
            Config &option_struct = std::get<Config>(_config_variants);
            config_group option_group = opt_group(custom_options, option_struct.group_name());
            option_struct.configure(option_group);
        }

        std::tuple<Configurables...> _config_variants;

    };

} //ns as
