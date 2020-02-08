//TODO splitter and quorum tests

//TODO first try to get this building!


#include <iostream>
#include <vector>

#include "gtest/gtest.h"

#include "caf/all.hpp"
#include "caf/io/all.hpp"

#include "common/splitter.hpp"
#include "common/event-handler.hpp"

// Simple action for multiplying arg by a specific pre-set constant
struct multiply_action_t {
    using result_t = int;
    int arg;
};

CAF_ALLOW_UNSAFE_MESSAGE_TYPE(multiply_action_t);

struct parent_action_t {
    using result_t = int;
    std::vector<int> args;
};

CAF_ALLOW_UNSAFE_MESSAGE_TYPE(parent_action_t);

class good_actor : public caf::event_handler<multiply_action_t> {

public:

    good_actor(caf::actor_config &a_cfg, int const_arg) : caf::event_handler<multiply_action_t>(a_cfg), _const_arg(const_arg) {}

    virtual caf::result<int> handle(const multiply_action_t &ma) override {
        return _const_arg * ma.arg;
    }

private:

    int _const_arg;

};

class bad_actor : public caf::event_handler<multiply_action_t> {
public:

    bad_actor(caf::actor_config &a_cfg) : caf::event_handler<multiply_action_t>(a_cfg) {}

    virtual caf::result<int> handle(const multiply_action_t &ma) override {
        throw std::logic_error("This always goes wrong!");
    }

};



class all_good_case : public caf::event_handler<parent_action_t> {

public:

    all_good_case(caf::actor_config &a_cfg, std::size_t split_size = 0, int init = 0, int multiplier = 42)
        : caf::event_handler<parent_action_t>(a_cfg), _split_size(split_size), _init(init), _multiplier(multiplier) {}

    virtual caf::result<int> handle(const parent_action_t &pa) override {
        // TODO ... would be good to infer the class type so we just need the return type!
        auto my_splitter = as::common::splitter<all_good_case, int>(this, _split_size, _init);
        auto delegated_actor = system().spawn<good_actor>(_multiplier);
        for(auto arg : pa.args) {
            auto action = multiply_action_t{arg};
            my_splitter.delegate(delegated_actor, action);
        }
        return my_splitter.make_response_promise();
    }

private:

    int _split_size;
    int _init;
    int _multiplier;

};

//TODO we need to

TEST(SplitterTests, SimpleReducer) {
    caf::actor_system_config cfg;
    cfg.load<caf::io::middleman>();
    caf::actor_system system{cfg};

    auto main_hdl = system.spawn<all_good_case>(3, 0, 42);
    auto main_hdl_fn = caf::make_function_view(main_hdl);
    auto test_action1 = parent_action_t{{1, 2, 3}};
    auto result = main_hdl_fn(test_action1);
    EXPECT_EQ((0 + 1 * 42 + 2 * 42 + 3 * 42), main_hdl_fn(test_action1));

}

TEST(SplitterTests, SimpleInitialValue) {
    caf::actor_system_config cfg;
    cfg.load<caf::io::middleman>();
    caf::actor_system system{cfg};

    // Testing with a non-trivial initial value
    auto main_hdl = system.spawn<all_good_case>(4, -145, 5);
    auto main_hdl_fn = caf::make_function_view(main_hdl);
    auto test_action1 = parent_action_t{{4, 5, 0, 7}};
    auto result = main_hdl_fn(test_action1);
    EXPECT_EQ((-145 + 4 * 5 + 5 * 5 + 7 * 5), main_hdl_fn(test_action1));

}
//TODO need to test that we just finish right away!
TEST(SplitterTests, TestZeroSplitSplitter) {
    caf::actor_system_config cfg;
    cfg.load<caf::io::middleman>();
    caf::actor_system system{cfg};

    //TODO try a spawn... OK let's now wrap it in a function handler! TODO TODO
    auto main_hdl = system.spawn<all_good_case>();
    auto main_hdl_fn = caf::make_function_view(main_hdl);
    auto test_action1 = parent_action_t{{1, 2, 3}};
    //main_hdl_fn(test_action1);

}

//TODO this is quite important...
TEST(SplitterTests, TestFailureToDelegateEnoughTimeout) {
    caf::actor_system_config cfg;
    cfg.load<caf::io::middleman>();
    caf::actor_system system{cfg};
}

//TODO: we now need one with everything bad


//TODO: we need a multi_good with multiple spawns.

//TODO: we then need a mix of good and bad