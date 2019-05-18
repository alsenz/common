load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")


# sole (for uuids!)
http_archive(
    name = "sole",
    build_file = "@//external:sole.BUILD",
    strip_prefix = "sole-653a25ad03775d7e0a2d50142160795723915ba6",
    urls = ["https://github.com/r-lyeh-archived/sole/archive/653a25ad03775d7e0a2d50142160795723915ba6.zip"],
)

# nlohmann
http_archive(
    name = "nlohmann",
    build_file = "@//external:nlohmann.BUILD",
    strip_prefix = "json-3.3.0",
    urls = ["https://github.com/nlohmann/json/archive/v3.3.0.tar.gz"],
)

# Google test
http_archive(
    name = "gtest",
    strip_prefix = "googletest-release-1.8.1",
    urls = ["https://github.com/google/googletest/archive/release-1.8.1.tar.gz"],
)

# Caf dependency
http_archive(
    name = "libcaf",
    build_file = "@//external:libcaf.BUILD",
    strip_prefix = "actor-framework-0.16.3",
    urls = ["https://github.com/actor-framework/actor-framework/archive/0.16.3.tar.gz"],
)
