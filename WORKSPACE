load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)
load(
    "@bazel_tools//tools/build_defs/repo:git.bzl",
    "git_repository",
    "new_git_repository",
)

http_archive(
    name = "gtest",
    sha256 = "9bf1fe5182a604b4135edc1a425ae356c9ad15e9b23f9f12a02e80184c3a249c",
    strip_prefix = "googletest-release-1.8.1",
    urls = ["https://github.com/google/googletest/archive/release-1.8.1.tar.gz"],
)

http_archive(
    name = "libev",
    build_file = "@//:libev.build",
    sha256 = "2d5526fc8da4f072dd5c73e18fbb1666f5ef8ed78b73bba12e195cfdd810344e",
    strip_prefix = "libev-4.27",
    urls = ["http://dist.schmorp.de/libev/Attic/libev-4.27.tar.gz"],
)

http_archive(
    name = "httpparser",
    build_file = "@//:httpparser.build",
    sha256 = "5199500e352584852c95c13423edc5f0cb329297c81dd69c3c8f52a75496da08",
    strip_prefix = "http-parser-2.9.2",
    urls = ["https://github.com/nodejs/http-parser/archive/v2.9.2.tar.gz"],
)

git_repository(
    name = "boringssl",
    commit = "e0c35d6c06fd800de1092f0b4d4326570ca2617a",
    remote = "https://boringssl.googlesource.com/boringssl",
    shallow_since = "1566966435 +0000",
)

http_archive(
    name = "absl",
    sha256 = "8100085dada279bf3ee00cd064d43b5f55e5d913be0dfe2906f06f8f28d5b37e",
    strip_prefix = "abseil-cpp-20190808",
    urls = ["https://github.com/abseil/abseil-cpp/archive/20190808.tar.gz"],
)