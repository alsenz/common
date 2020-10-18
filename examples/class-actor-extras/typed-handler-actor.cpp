
#include <iostream>
#include <string>

#include "caf/all.hpp"
#include "common/virtual-handler.hpp"

struct event_1 {
    using result_t = int;
    char data;
};

struct event_2 {
    char data;
};

//TODO this should not work because we should be missing a virtual override!
class test_class_actor : public caf::handler_base<caf::reacts_to<std::string>, caf::replies_to<float>::with<bool>> {

public:

    //TODO make the result type void
    virtual void handle(const std::string &arg) override {}

    virtual caf::result<bool> handle(const float &f) override { return false; }

};



auto main() -> int {
    std::cout << "testin 123" << std::endl;

    //TODO try mockin' up some classes here

    test_class_actor xyz;

    //TODO let's actually spawn these things...

    return 0;
}
