cc_library(
    name = "common",
    srcs = glob(["src/*.cpp"]),
    hdrs = glob([
        "src/*.hpp",
        "src/nonstd/span.hpp",
    ]),
    copts = ["-std=c++17"],
    linkopts = ["-lstdc++fs"],
    visibility = ["//visibility:public"],
    deps = [
        "@libcaf",
        "@nlohmann//:json",
        "@sole",
    ],
    strip_include_prefix = "src",
    include_prefix = "common"
)

cc_test(
    name = "tests",
    srcs = glob(["tests/*.cpp"]),
    copts = [
        "-std=c++17",
        "-Wno-write-strings",
    ],
    linkopts = ["-lstdc++fs"],
    deps = [
        ":common",
        "@gtest//:gtest_main",
        "@nlohmann//:json",
        "@sole",
    ],
    visibility = ["//visibility:public"],
)


cc_library(
    name = "lib-tests",
    srcs = glob(["tests/*.cpp"]),
    copts = [
        "-std=c++17",
        "-Wno-write-strings",
    ],
    linkopts = ["-lstdc++fs"],
    deps = [
        ":common",
        "@gtest//:gtest_main",
        "@nlohmann//:json",
        "@sole",
    ],
    visibility = ["//visibility:public"],
)
