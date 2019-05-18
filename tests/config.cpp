//
// Created by tom on 13/10/18.
//

#include <iostream>

#include "gtest/gtest.h"

#include "caf/all.hpp"
#include "caf/io/all.hpp"

#include "common/config.hpp"

struct test_config {
    std::string hostname = "localhost";
    unsigned int port = 8018;
    std::string name = "as-http-server";

    void configure(as::config_group &group) {
        group.add(hostname, "hostname", "server hostname");
        group.add(port, "port", "server port");
        group.add(name, "name", "server name");
    }

    constexpr const char *group_name() const {
        return "test-config";
    }

};

TEST(CommonConfigTests, TestHelptextPrinted) {

    char *argv[] = {"snazzyd", "--help", NULL};
    int argc = sizeof(argv) / sizeof(char*) - 1;

    as::config<test_config> cfg;
    cfg.load<caf::io::middleman>(); // Just to check we can still load etc.
    cfg.parse(argc, argv, "/etc/as-apid.ini");

    EXPECT_TRUE(cfg.cli_helptext_printed);

}

TEST(CommonConfigTests, TestCustomConfigExtensions) {

    char *argv[] = {"snazzyd", "--test-config.hostname=hostymchostface.com", "--test-config.port=5432", NULL};
    int argc = sizeof(argv) / sizeof(char*) - 1;

    as::config<test_config> cfg;
    cfg.load<caf::io::middleman>(); // Just to check we can still load etc.
    cfg.parse(argc, argv, "/etc/as-apid.ini");

    EXPECT_FALSE(cfg.cli_helptext_printed);
    EXPECT_EQ("hostymchostface.com", cfg.get<test_config>().hostname);
    EXPECT_EQ(5432, cfg.get<test_config>().port); //Note that it's an int
    EXPECT_EQ("as-http-server", cfg.get<test_config>().name); //Note that this is a default!

}
