#include <iostream>
#include <vector>

#include "gtest/gtest.h"

#include "caf/all.hpp"
#include "caf/io/all.hpp"

#include "common/splitter.hpp"
#include "common/event-handler.hpp"

#include <chrono>

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
        std::cout << "Returning a caf error..." << std::endl;
        return caf::make_error(caf::sec::runtime_error);
    }

};



class all_good_case : public caf::event_handler<parent_action_t> {

public:

    all_good_case(caf::actor_config &a_cfg, std::size_t split_size = 0, int init = 0, int multiplier = 42)
        : caf::event_handler<parent_action_t>(a_cfg), _split_size(split_size), _init(init), _multiplier(multiplier) {}

    virtual caf::result<int> handle(const parent_action_t &pa) override {
        // TODO ... would be good to infer the class type from "this" by default so we just need the return type!
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

TEST(SplitterTests, TestZeroSplitSplitter) {
    caf::actor_system_config cfg;
    cfg.load<caf::io::middleman>();
    caf::actor_system system{cfg};

    auto main_hdl = system.spawn<all_good_case>(0);
    auto main_hdl_fn = caf::make_function_view(main_hdl);
    auto test_action1 = parent_action_t{{}};
    // Check that we fire back straight away!
    EXPECT_EQ(0, main_hdl_fn(test_action1));

}


//TODO just move these into the tests to make the tests more manageable
class all_good_case_multi_spawn : public caf::event_handler<parent_action_t> {

public:

    all_good_case_multi_spawn(caf::actor_config &a_cfg, std::size_t split_size = 0, int init = 0, int multiplier = 42)
        : caf::event_handler<parent_action_t>(a_cfg), _split_size(split_size), _init(init), _multiplier(multiplier) {}

    virtual caf::result<int> handle(const parent_action_t &pa) override {
        auto my_splitter = as::common::splitter<all_good_case_multi_spawn, int>(this, _split_size, _init);
        for(auto arg : pa.args) {
            auto forked_actor = system().spawn<good_actor>(_multiplier);
            auto action = multiply_action_t{arg};
            my_splitter.delegate(forked_actor, action);
        }
        return my_splitter.make_response_promise();
    }

private:

    int _split_size;
    int _init;
    int _multiplier;

};

TEST(SplitterTests, MultiSpawnSimpleReducer) {
    caf::actor_system_config cfg;
    cfg.load<caf::io::middleman>();
    caf::actor_system system{cfg};

    auto main_hdl = system.spawn<all_good_case_multi_spawn>(3, 0, 42);
    auto main_hdl_fn = caf::make_function_view(main_hdl);
    auto test_action1 = parent_action_t{{1, 2, 3}};
    auto result = main_hdl_fn(test_action1);
    EXPECT_EQ((0 + 1 * 42 + 2 * 42 + 3 * 42), main_hdl_fn(test_action1));

}


//TODO let's try a simple error-handling case...
TEST(SplitterTests, SimpleErrorHandling) {

    //TODO let's inline these tests a bit, make them make more sense..

    caf::actor_system_config cfg;
    cfg.load<caf::io::middleman>();
    caf::actor_system system{cfg};

    struct all_bad_case : public caf::event_handler<parent_action_t> {

        using caf::event_handler<parent_action_t>::event_handler;

        virtual caf::result<int> handle(const parent_action_t &pa) override {
            //TODO use template argument deduction on the this parameter in the constructor (and then maybe store a pointer!)
            //TODO storing as a strong actor pointer may actually *help* our segfault issue? TODO think about this...
            //TODO we need to make this a timeout splitter now.
            auto my_splitter = as::common::splitter<all_bad_case, int>(this, 3, 123);
            for(auto arg : pa.args) {
                auto forked_actor = system().spawn<bad_actor>();
                auto action = multiply_action_t{arg};
                my_splitter.delegate(forked_actor, action);
            }
            return my_splitter.make_response_promise();
        }

    };

    auto main_hdl = system.spawn<all_bad_case>();
    auto main_hdl_fn = caf::make_function_view(main_hdl);
    auto test_action = parent_action_t{{1,2,3}};
    auto result = main_hdl_fn(test_action);
    EXPECT_EQ((123 + 5 * 42 + 6 * 42 + 7 * 42), result);


}


//TODO build this more into a lambda style spawn to avoid the extra gumpf writing tests.
class all_good_case_multi_spawn_timeout : public caf::event_handler<parent_action_t> {

public:

    all_good_case_multi_spawn_timeout(caf::actor_config &a_cfg, std::size_t split_size = 0, int init = 0, int multiplier = 42, std::chrono::milliseconds timeout_millis = std::chrono::milliseconds(0))
        : caf::event_handler<parent_action_t>(a_cfg), _split_size(split_size), _init(init), _multiplier(multiplier), _timeout_millis(timeout_millis) {}


    virtual caf::result<int> handle(const parent_action_t &pa) override {

        auto my_splitter = as::common::timed_splitter<all_good_case_multi_spawn_timeout, int>(this, _split_size,
                                                                                              _init,
                                                                                              _timeout_millis);
        for (auto arg : pa.args) {
            auto forked_actor = system().spawn<good_actor>(_multiplier);
            auto action = multiply_action_t{arg};
            my_splitter.delegate(forked_actor, action);
        }

        std::cout << "Returning response promise..." << std::endl;
        return my_splitter.make_response_promise();

    }

    virtual ~all_good_case_multi_spawn_timeout() {
        std::cout << "Deleting the class actor underneath this..." << std::endl;
    }


private:

    int _split_size;
    int _init;
    int _multiplier;
    std::chrono::milliseconds _timeout_millis;

};


/*
TEST(SplitterTests, TestSimpleTimeoutCase) {
    caf::actor_system_config cfg;
    cfg.load<caf::io::middleman>();
    caf::actor_system system{cfg};
    // Note: we make the timeout here really slow so that we can spot the bug that we're waiting for the timeout to elapse
    auto main_hdl = system.spawn<all_good_case_multi_spawn_timeout>(5, 123, 321, std::chrono::milliseconds(10000));
    auto main_hdl_fn = caf::make_function_view(main_hdl);

    auto test_action1 = parent_action_t{{1, 2, 3, 4, 5}};
    std::cout << main_hdl_fn(test_action1) << std::endl;
    EXPECT_EQ((123 + 1 * 321 + 2 * 321 + 3 * 321 + 4 * 321 + 5 * 321), main_hdl_fn(test_action1));

    auto test_action2 = parent_action_t{{1, 2, 3}}; //Not enough!
    //TODO: me thinks we have a segfault!
    std::cout << "A" << std::endl;
    auto res2 = main_hdl_fn(test_action2);
    std::cout << "B" << std::endl;
    std::cout << res2 << std::endl;
    std::cout << "C" << std::endl;
    std::cout << "We nearly got 'der" << std::endl;
    //EXPECT_EQ((123 + 1 * 321 + 2 * 321 + 3 * 321 + 4 * 321 + 5 * 321), main_hdl_fn(test_action2));
    std::cout << "We got 'der" << std::endl;

    // We should test how a timeout is handled...

}
 */

//TODO: we now need one with everything bad //TODO TODO


//TODO: we need a multi_good with multiple spawns.

//TODO: we then need a mix of good and bad