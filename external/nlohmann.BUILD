licenses(["notice"])  # 3-Clause BSD

exports_files(["LICENSE.MIT"])

cc_library(
    name = "json",
    hdrs = glob([
        "include/nlohmann/**/*.hpp",
    ]),
    copts = [
        "-I external/nlohmann",
    ],
    visibility = ["//visibility:public"],
    alwayslink = 1,
    strip_include_prefix = "include"
)
