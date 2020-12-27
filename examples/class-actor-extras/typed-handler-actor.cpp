
#include <iostream>
#include <string>

#include "caf/all.hpp"
#include "caf/io/all.hpp"

#include "common/typed-handler-actor.hpp"
#include "common/handles.hpp"
#include "common/config.hpp"
#include "common/automatic-typeids.hpp"

struct event_26 {
    using result_t = int;
    char data;
};

ENABLE_CAF_TYPE(event_26);

template <class Inspector>
typename Inspector::result_type inspect(Inspector &f, event_26 &x) {
    return f(caf::meta::type_name("event_26"), x.data);
}

struct event_42 {
    using result_t = float;
    int data;
};

ENABLE_CAF_TYPE(event_42);

template <class Inspector>
typename Inspector::result_type inspect(Inspector &f, event_42 &x) {
    return f(caf::meta::type_name("event_42"), x.data);
}

class test_class_actor : public caf::typed_handler_actor<
    caf::handles<std::string>,
    caf::replies_to<float>::with<bool>,
    caf::handles<event_26>,
    caf::replies_posthoc_to<int>::with<std::string>,
    caf::handles_posthoc<event_42>
    > {

public:

    using super = caf::typed_handler_actor<
        caf::reacts_to<std::string>,
        caf::replies_to<float>::with<bool>,
        caf::handles<event_26>,
        caf::replies_posthoc_to<int>::with<std::string>,
        caf::handles_posthoc<event_42>
        >;

    struct config {

        using message_types = std::tuple<event_26, event_42>;

        //TODO make this removable
        void configure(as::config_group &group) {
            //Nothing
        }

        //TODO make this removable
        constexpr const char *group_name() const {
            return "todo-remove";
        }

    };

    test_class_actor(caf::actor_config &cfg) : super(cfg) {}

    virtual void handle(const std::string &arg) override {
        std::cout << "Handling string argument " << arg << std::endl;
    }

    virtual bool handle(const float &f) override {
        return false;
    }

    virtual event_26::result_t handle(const event_26 &e) override {
        return 42;
    }

    virtual caf::result<std::string> handle(const int &i) override {
        return std::string("abc");
    }

    virtual caf::result<event_42::result_t> handle(const event_42 &e) override {
        return 123.456;
    }

};



auto main(int argc, char** argv) -> int {

    as::config<test_class_actor::config> cfg;
    cfg.load<caf::io::middleman>(); //TODO make this work without stuff to config..
    cfg.init_meta_objects();
    auto err = cfg.parse(argc, argv, "/etc/gandt.ini");
    if(cfg.cli_helptext_printed) {
        return 0; //We're done.b
    }

    caf::actor_system system{cfg};
    if(err) {
        std::cerr << caf::to_string(err) << std::endl;
        return 0;
    }

    auto test_actor = system.spawn<test_class_actor>();
    auto test_actor_fn = caf::make_function_view(test_actor);

    std::cout << "Result of a float is: " << (test_actor_fn((float) 43.24) ? "true" : "flase") << std::endl;
    std::cout << "Handling string arg..." << std::endl;
    test_actor_fn(std::string("hello there"));
    std::cout << "Handling handles with event...." << std::endl;
    auto evt = event_26{'a'};
    auto code = test_actor_fn(evt).value();
    std::cout << "Code from event: " << code << std::endl;

    return 0;
}
