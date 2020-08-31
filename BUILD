cc_library(
    name = "common",
    srcs = glob(["src/*.cpp"]),
    hdrs = glob([
        "src/*.hpp",
        "src/nonstd/span.hpp",
    ]),
    copts = [
        "-std=c++17",
        "-fconcepts",
    ],
    include_prefix = "common",
    linkopts = ["-lstdc++fs"],
    strip_include_prefix = "src",
    visibility = ["//visibility:public"],
    deps = [
        "@libcaf",
        "@nlohmann//:json",
        "@sole",
    ],
)

cc_test(
    name = "tests",
    srcs = glob(["tests/*.cpp"]),
    copts = [
        "-std=c++17",
        "-Wno-write-strings",
        "-fconcepts",
    ],
    linkopts = ["-lstdc++fs"],
    visibility = ["//visibility:public"],
    deps = [
        ":common",
        "@gtest//:gtest_main",
        "@nlohmann//:json",
        "@sole",
    ],
)

cc_library(
    name = "lib-tests",
    srcs = glob(["tests/*.cpp"]),
    copts = [
        "-std=c++17",
        "-Wno-write-strings",
        "-fconcepts",
    ],
    linkopts = ["-lstdc++fs"],
    visibility = ["//visibility:public"],
    deps = [
        ":common",
        "@gtest//:gtest_main",
        "@nlohmann//:json",
        "@sole",
    ],
)


############# Examples

cc_binary(
    name = "typed-handler-actor-example",
    srcs = ["examples/class-actor-extras/typed-handler-actor.cpp"],
    copts = [
            "-std=c++17",
            "-Wno-write-strings",
            "-fconcepts",
    ],
    linkopts = ["-lstdc++fs"],
    deps = [
        ":common"
    ],
)
