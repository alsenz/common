#pragma once

#include <type_traits>
#include <sstream>
#include <tuple>
#include <iostream>

#include "caf/all.hpp"

#include "common/virtual-handler.hpp"
#include "common/reducer-mixin.hpp"
#include "common/logger.hpp"
#include "common/handles.hpp"

namespace caf {

    // The idea here is to provide an actor base class which automatically sets up virtual handle(...) member functions for behaviours given a type signature

    namespace detail2 { //TODO change this to detail when event-handler.hpp is deprecated.

        template<typename T>
        struct behaviors_to_handler;

        template<typename... Os, typename... Is>
        struct behaviors_to_handler<caf::result<Os...>(Is...)> {
            template<typename T>
            auto operator()(T *thiz) {
                return [thiz](const Is &... args) -> detail::first_pack_type_t<Os...> {
                    std::cout << "A 1" << std::endl;
                    std::cout << typeid(caf::result<Os...>).name() << std::endl;
                    std::cout << typeid(caf::result<void>).name() << std::endl;
                    std::cout << typeid(caf::result<int>).name() << std::endl;
                    //as::common::logger::trace_a(thiz, ss.str(), "event-handler", __FUNCTION__, __LINE__, __FILE__);
                    //auto res = thiz->handle(std::forward<Is>(args)...);
                    auto res = thiz->handle(args...);
                    std::cout << "A 2" << std::endl;
                    return res;
                };
            }
        };

        template<typename... Os, typename... Is>
        struct behaviors_to_handler<opaque_result<Os...>(Is...)> {
            template<typename T>
            auto operator()(T *thiz) {
                return [thiz](Is... args) -> caf::result<detail::first_pack_type_t<Os...>> {
                    //as::common::logger::trace_a(thiz, ss.str(), "event-handler", __FUNCTION__, __LINE__, __FILE__);
                    std::cout << "B 1" << std::endl;
                    auto res = thiz->handle(std::forward<Is>(args)...);
                    std::cout << "B 2" << std::endl;
                    return res;
                };
            }
        };

        template<typename... Is>
        struct behaviors_to_handler<caf::result<void>(Is...)> {

            template<typename T>
            auto operator()(T *thiz) {
                return [thiz](Is... args) -> void {
                    std::cout << "C 1" << std::endl;
                    thiz->handle(std::forward<Is>(args)...);
                    std::cout << "C 2" << std::endl;
                };
            }
        };

    }


    template<typename... Sigs>
    class typed_handler_actor;

    namespace detail {

        template<typename... Ts>
        struct strip_opaque;

        template<typename... Os, typename... Is>
        struct strip_opaque<result<Os...>(Is...)> {
            using type = result<Os...>(Is...);
        };

        template<typename... Os, typename... Is>
        struct strip_opaque<opaque_result<Os...>(Is...)> {
            using type = result<Os...>(Is...);
        };

        template<typename... Ts>
        using strip_opaque_t = typename strip_opaque<Ts...>::type;

        template<typename... Sigs>
        using typed_handler_actor_base = mixin::reducer<typename typed_actor<strip_opaque_t<Sigs>...>::base, typed_handler_actor<Sigs...>>;

    } //ns detail

    template<typename... Sigs>
    class typed_handler_actor
    : public detail::typed_handler_actor_base<Sigs...>, //Add the reducer mixin for convenience
          public handler_base<Sigs...> {

    public:

        //using super = typename mixin::reducer<typename typed_actor<Sigs...>::base, typed_handler_actor<Sigs...>>;
        using super = detail::typed_handler_actor_base<Sigs...>;

        template<typename SingleClause>
        friend class detail2::behaviors_to_handler;

        typed_handler_actor(caf::actor_config &cfg) : super(cfg) {}

        // This basically just eliminates the boiler plate for make_behavior, and the spelling.
        typename super::behavior_type make_behavior() override {

            //TODO set default handler appears broken... TODO TODO Fixme
            /*super::set_default_handler([this](caf::scheduled_actor *self_ptr, const caf::message &msg) mutable -> result<message> {
                std::stringstream ss;
                ss << "Unexpected message [receiver id: " << self_ptr->id();
                ss << ", name: " << self_ptr->name();
                ss << "]: " << caf::to_string(msg);
                LOG_ERROR_A("event-handler", ss.str());
                on_unexpected_message(msg);
                return sec::unexpected_message;
            });*/

            super::set_down_handler([this](caf::scheduled_actor *self_ptr, const caf::down_msg &msg) mutable {
                std::stringstream ss;
                ss << "Handler going down [id: " << self_ptr->id();
                ss << ", name: " << self_ptr->name();
                ss << "]: " << to_string(msg);
                LOG_WARN_A("event-handler", ss.str());
                on_down_msg(msg);
            });

            super::set_exit_handler([this](caf::scheduled_actor *self_ptr, const caf::exit_msg &msg) mutable {
                std::stringstream ss;
                ss << "Received exit message [id: " << self_ptr->id();
                ss << ", name: " << self_ptr->name();
                if(msg.reason) {
                    ss << ", reason: " << caf::to_string(msg.reason);
                }
                ss << "]: " << to_string(msg);
                LOG_WARN_A("event-handler", ss.str());
                on_exit(msg);
            });

            return {
                detail2::behaviors_to_handler<Sigs>()(this)...
            };
        }


    protected:

        virtual void on_unexpected_message(const caf::message &msg) {}

        virtual void on_down_msg(const caf::down_msg &msg) {}

        virtual void on_exit(const caf::exit_msg &msg) {
            super::quit(); //Fixme: handle the exist message
        }

    };

} //ns caf