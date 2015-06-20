cc_library(
  name = "expressions-lib",
  srcs = [
    "textsource.cc",
    "tokenizer.cc",
    "expression.cc",
  ],
  hdrs = [
    "textsource.h",
    "tokenizer.h",
    "expression.h",
  ],
)

cc_binary(
  name = "expr",
  srcs = ["expr.cc"],
  deps = [
    ":expressions-lib",
  ],
)

cc_test(
  name = "expression_test",
  srcs = ["expression_test.cc"],
  deps = [
    ":expressions-lib",
    "//gtest:gtest_main",
  ],
)

cc_test(
  name = "textsource_test",
  srcs = ["textsource_test.cc"],
  deps = [
    ":expressions-lib",
    "//gtest:gtest_main",
  ],
)

cc_test(
  name = "tokenizer_test",
  srcs = ["tokenizer_test.cc"],
  deps = [
    ":expressions-lib",
    "//gtest:gtest_main",
  ],
)
