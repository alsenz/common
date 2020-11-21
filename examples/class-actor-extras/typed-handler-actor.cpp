
#include <iostream>
#include <string>

#include "caf/all.hpp"
#include "caf/io/all.hpp"

#include "common/typed-handler-actor.hpp"
#include "common/handles.hpp"
#include "common/config.hpp"
#include "common/common-typeids.hpp"

struct event_26 {
    using result_t = int;
    char data;
};

struct event_42 {
    using result_t = float;
    int data;
};

CAF_BEGIN_TYPE_ID_BLOCK(example_typed_handler_actor, gt::common::custom_type_block_end + 50)
    CAF_ADD_TYPE_ID(example_typed_handler_actor, (event_26))
    CAF_ADD_TYPE_ID(example_typed_handler_actor, (event_42))
CAF_END_TYPE_ID_BLOCK(example_typed_handler_actor)

class test_class_actor : public caf::typed_handler_actor<
    caf::reacts_to<std::string>,
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

    as::config<> cfg;
    cfg.load<caf::io::middleman>();
    auto err = cfg.parse(argc, argv, "/etc/gandt.ini");
    if(cfg.cli_helptext_printed) {
        return 0; //We're done.b
    }

    caf::core::init_global_meta_objects();
    caf::io::middleman::init_global_meta_objects();
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
    auto code = test_actor_fn(event_26{'a'}).value();
    std::cout << "Code from event: " << code << std::endl;

    return 0;
}
