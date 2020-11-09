COMMON_COPTS = [
    "-std=c++17",
    "-fconcepts",
    "-Wno-write-strings",
]

COMMON_LINKOPTS = [
    "-lstdc++fs",
    "-lpthread",
]

COMMON_DEPS = [
    "@libcaf",
    "@nlohmann//:json",
    "@sole",
]

cc_library(
    name = "common-headers",
    hdrs = glob([
        "common/*.hpp",
        "common/nonstd/span.hpp",
    ]),
    copts = COMMON_COPTS,
    linkopts = COMMON_LINKOPTS,
    visibility = ["//visibility:public"],
    deps = COMMON_DEPS,
    defines = ["span_FEATURE_COMPARISON", "span_FEATURE_SAME", "span_FEATURE_MEMBER_SWAP"]
)

cc_library(
    name = "common",
    srcs = glob(["src/*.cpp"]),
    copts = COMMON_COPTS,
    include_prefix = "common",
    linkopts = COMMON_LINKOPTS,
    strip_include_prefix = "src",
    visibility = ["//visibility:public"],
    deps = COMMON_DEPS + [":common-headers"],
)

cc_test(
    name = "tests",
    srcs = glob(["tests/*.cpp"]),
    copts = COMMON_COPTS,
    linkopts = ["-lstdc++fs"],
    visibility = ["//visibility:public"],
    deps = COMMON_DEPS + [
        ":common",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "lib-tests",
    srcs = glob(["tests/*.cpp"]),
    copts = COMMON_COPTS,
    linkopts = COMMON_LINKOPTS,
    visibility = ["//visibility:public"],
    deps = COMMON_DEPS + [
        ":common",
        "@gtest//:gtest_main",
    ],
)

############# Examples

cc_binary(
    name = "typed-handler-actor-example",
    srcs = ["examples/class-actor-extras/typed-handler-actor.cpp"],
    copts = COMMON_COPTS,
    linkopts = COMMON_LINKOPTS,
    deps = [":common"],
)
