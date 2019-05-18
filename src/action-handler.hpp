#pragma once

#include <type_traits>
#include <sstream>
#include <tuple>

#include "caf/all.hpp"

#include "common/logger.hpp"
#include "common/throwaway-action.hpp"

// This is so close to caf functionality, we're just going to hijack the namespace a bit
namespace caf {


    namespace detail {

        template<typename Action>
        using action_sig = std::conditional_t<
            std::is_void_v<typename Action::result_t>,
            typename caf::reacts_to<Action>,
            typename caf::replies_to<Action>::template with<typename Action::result_t>>;

        template<typename... Actions>
        using action_typed_actor = typed_actor<action_sig<Actions>...>;

        template<typename T>
        struct behaviors_to_handler;

        template<typename O, typename... Is>
        struct behaviors_to_handler<typed_mpi < detail::type_list<Is...>, output_tuple <O>>> {
            template<typename T>
            auto operator()(T *thiz) {
                return [thiz](Is... args) -> caf::result<O> {
                    std::stringstream ss;
                    ss << "Handling action with result. This type: " << typeid(T).name() << ", result type: " << typeid(O).name() << ", args tuple: " << typeid(std::tuple<Is...>).name();
                    as::common::logger::trace_a(thiz, ss.str(), "action-handler", __FUNCTION__, __LINE__, __FILE__);
                    return thiz->handle(std::forward<Is>(args)...);
                };
            }
        };

        template<typename... Is>
        struct behaviors_to_handler<typed_mpi<detail::type_list<Is...>, output_tuple<void>>> {
            template<typename T>
            auto operator()(T *thiz) {
                return [thiz](Is... args) -> void {
                    std::stringstream ss;
                    ss << "Handling action with no result (void)! This type: " << typeid(T).name() << ", args tuple: " << typeid(std::tuple<Is...>).name();
                    as::common::logger::trace_a(thiz, ss.str(), "action-handler", __FUNCTION__, __LINE__, __FILE__);
                    thiz->handle(std::forward<Is>(args)...);
                };
            }
        };

        template<typename T>
        struct behaviors_to_throw_away_handler;

        template<typename... Is>
        struct behaviors_to_throw_away_handler<typed_mpi<detail::type_list<Is...>, output_tuple<void>>> {
            template<typename T>
            auto operator()(T *thiz) {
                return [thiz](Is... args) -> void {
                    std::stringstream ss;
                    ss << "Handling throwaway action. This type: " << typeid(T).name() << ", args tuple: " << typeid(std::tuple<Is...>).name();
                    as::common::logger::trace_a(thiz, ss.str(), "action-handler", __FUNCTION__, __LINE__, __FILE__);
                    // Basically call the handle on the base action type for the throw away wrapper.
                    thiz->handle(((const typename Is::base_action_t &) args)...);
                };
            }
        };

    } // ns detail


    // An action handler is a statically typed actor that has a "handle" method for each Action. Action must be a struct with a typedef called result_t, with a result type
    // each handle method must exist for each action and have the signature caf::result<Action::result_t> handle(Action)!

    // The action handler pattern has the advantage that 1) there is no need to manually call make_behavior() (with the wrong spelling!) and
    // 2) the method signatures are a darn-sight clearer and more obvious.

    // As with most cases of actor design, no effort it made to support inheritance.

    // Finally, each action has a throw_away action variant, which is a special type wrapper that causes the action to be handled as before, but ignores the result.

    template<typename Derived, typename... Actions>
    class action_handler : public detail::action_typed_actor<Actions..., throw_away_action_t<Actions>...>::base {

    public:

        using super = typename detail::action_typed_actor<Actions..., throw_away_action_t<Actions>...>::base;

        template<typename SingleClause>
        friend class detail::behaviors_to_handler;

        action_handler(caf::actor_config &cfg) : super(cfg) {}

        // This basically just eliminates the boiler plate for make_behavior, and the spelling.
        typename super::behavior_type make_behavior() override {
            super::set_default_handler([this](caf::scheduled_actor *self_ptr, const caf::message_view &msg) mutable -> result<message> {
                std::stringstream ss;
                ss << "Unexpected message [receiver id: " << self_ptr->id();
                ss << ", name: " << self_ptr->name();
                ss << ", derived type: " << typeid(Derived).name();
                ss << "]: " << msg.content().stringify();
                LOG_ERROR_A("action-handler", ss.str());
                on_unexpected_message(msg);
                return sec::unexpected_message;
            });

            super::set_down_handler([this](caf::scheduled_actor *self_ptr, const caf::down_msg &msg) mutable {
                std::stringstream ss;
                ss << "Handler going down [id: " << self_ptr->id();
                ss << ", name: " << self_ptr->name();
                ss << ", derived type: " << typeid(Derived).name();
                ss << "]: " << to_string(msg);
                LOG_WARN_A("action-handler", ss.str());
                on_down_msg(msg);
            });

            super::set_exit_handler([this](caf::scheduled_actor *self_ptr, const caf::exit_msg &msg) mutable {
                std::stringstream ss;
                ss << "Received exit message [id: " << self_ptr->id();
                ss << ", name: " << self_ptr->name();
                ss << ", derived type: " << typeid(Derived).name();
                if(msg.reason) {
                    ss << ", reason: " << this->system().render(msg.reason);
                }
                ss << "]: " << to_string(msg);
                LOG_WARN_A("action-handler", ss.str());
                on_exit(msg);
            });

            return {
                detail::behaviors_to_throw_away_handler<detail::action_sig<throw_away_action_t<Actions>>>()(static_cast<Derived *>(this))...,
                detail::behaviors_to_handler<detail::action_sig<Actions>>()(static_cast<Derived *>(this))...
            };
        }


    protected:

        virtual void on_unexpected_message(const caf::message_view &msg) {}

        virtual void on_down_msg(const caf::down_msg &msg) {}

        virtual void on_exit(const caf::exit_msg &msg) {
            super::quit();
        }

    };


} //ns caf
