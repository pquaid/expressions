#include <iostream>
#include <sstream>
#include <memory>

#include "exception.h"
#include "expression.h"
#include "textsource.h"
#include "tokenizer.h"

#include "gtest/gtest.h"

// Helper functions: evaluate the given expression, and return a value.
Expression::Value Evaluate(const char* expr) {
  std::istringstream s(expr);
  Tokenizer tokenizer(new TextSource(s));
  tokenizer.next();
  std::unique_ptr<Expression> e(Expression::compile(tokenizer));
  POSTCONDITION(e != nullptr);

  ExecutionContext exe;
  return e->evaluate(exe);
}

double EvaluateDouble(const char* expr) {
  return Evaluate(expr).asNumber();
}

std::string EvaluateString(const char* expr) {
  return Evaluate(expr).asString();
}

bool EvaluateBool(const char* expr) {
  return Evaluate(expr).asBool();
}

// Compare the results of the expression with the same thing computed in C.
#define EVALUATE_DOUBLE(x) EXPECT_DOUBLE_EQ((x), EvaluateDouble( #x ))
#define EVALUATE_STRING(x) EXPECT_EQ((x), EvaluateString( #x ))
#define EVALUATE_BOOL(x) EXPECT_EQ((x), EvaluateBool( #x ))

TEST(ExpressionTest, Errors) {
  EXPECT_THROW(EvaluateString("%6"), Exception);
  EXPECT_THROW(EvaluateString("6++"), Exception);
  EXPECT_THROW(EvaluateString("++6"), Exception);
  EXPECT_THROW(EvaluateString("4 = 2"), Exception);
  EXPECT_THROW(EvaluateString("4 4"), Exception);
  EXPECT_THROW(EvaluateDouble("true + true"), Exception);
}

TEST(ExpressionTest, Constants) {
  EXPECT_EQ(1.0, EvaluateDouble("1.0"));
  EXPECT_EQ(0xabc, EvaluateDouble("0xabc"));
  // FIXME: This should be interpreted as an octal int, but isn't because
  // I'm using strtod(). Given that bit operators exist, I probably want to
  // distinguish between ints and floats anyway.
  EXPECT_EQ(054.0, EvaluateDouble("054"));
  EXPECT_EQ(" str ", EvaluateString("' str '"));
  EXPECT_EQ("\nstr\n", EvaluateString("'\\nstr\\n'"));
  EXPECT_EQ(true, EvaluateBool("true"));
  EXPECT_EQ(false, EvaluateBool("false"));
}

TEST(ExpressionTest, NumberUnaryOperators) {
  EVALUATE_DOUBLE(!1);
  EVALUATE_DOUBLE(~1);
  EVALUATE_DOUBLE(-1);
  EVALUATE_DOUBLE(+1);
}

TEST(ExpressionTest, NumberBinaryOperators) {
  EVALUATE_DOUBLE(5 * 7);
  EVALUATE_DOUBLE(12 / 4);
  EVALUATE_DOUBLE(12 / -4);
  EVALUATE_DOUBLE(11 % 10);
  EVALUATE_DOUBLE(11 % -10);
  EVALUATE_DOUBLE(12 - 4);
  EVALUATE_DOUBLE(12 - -4);
  EVALUATE_DOUBLE(-12 - 4);
  EVALUATE_DOUBLE(12 + 4);
  EVALUATE_DOUBLE(12 + -4);
  EVALUATE_DOUBLE(-12 + 4);
  EVALUATE_DOUBLE(1 << 8);
  EVALUATE_DOUBLE(156 >> 3);
  EVALUATE_DOUBLE(127 & 48);
  EVALUATE_DOUBLE(48 | 1);
  EVALUATE_DOUBLE(5 ^ 31);
  EVALUATE_DOUBLE(4 && 0);
  EVALUATE_DOUBLE(4 || 0);

  EVALUATE_BOOL(5 < 7);
  EVALUATE_BOOL(5 > 7);
  EVALUATE_BOOL(5 <= 7);
  EVALUATE_BOOL(5 >= 7);
  EVALUATE_BOOL(5 <= 5);
  EVALUATE_BOOL(5 >= 5);
  EVALUATE_BOOL(5 == 5);
  EVALUATE_BOOL(5 != 5);
}

TEST(ExpressionTest, StringBinaryOperators) {
  EXPECT_EQ("foobar", EvaluateString("'foo' + 'bar'"));
  EXPECT_FALSE(EvaluateBool("'foo' < 'bar'"));
  EXPECT_TRUE(EvaluateBool("'foo' > 'bar'"));
  EXPECT_TRUE(EvaluateBool("'foo' >= 'bar'"));
  EXPECT_TRUE(EvaluateBool("'foo' >= 'foo'"));
  EXPECT_FALSE(EvaluateBool("'foo' <= 'bar'"));
  EXPECT_TRUE(EvaluateBool("'foo' <= 'foo'"));
  EXPECT_TRUE(EvaluateBool("'foo' == 'foo'"));
  EXPECT_FALSE(EvaluateBool("'foo' == 'bar'"));
  EXPECT_FALSE(EvaluateBool("'foo' != 'foo'"));
  EXPECT_TRUE(EvaluateBool("'foo' != 'bar'"));
}

TEST(ExpressionTest, BooleanBinaryOperators) {
  EVALUATE_BOOL(true == true);
  EVALUATE_BOOL(true == false);
  EVALUATE_BOOL(true != true);
  EVALUATE_BOOL(true != false);
  EVALUATE_BOOL(true && true);
  EVALUATE_BOOL(true && false);
  EVALUATE_BOOL(true || true);
  EVALUATE_BOOL(true || false);
}

TEST(ExpressionTest, TernaryOperator) {
  EVALUATE_DOUBLE(1 < 3 ? 2 : 4);
  EVALUATE_DOUBLE(1 > 3 ? 2 : 4);
}

TEST(ExpressionTest, Parentheses) {
  EVALUATE_DOUBLE(2 * (4 + 5));
  EVALUATE_DOUBLE(2 * 4 + 5);
  EVALUATE_DOUBLE((2 * 4) + 5);
}

TEST(ExpressionTest, Sequences) {
  // These values in these sequences are unused and obviously can generate
  // no side effects, so I need to disable the warning.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
  EXPECT_EQ((4, 5, 6), EvaluateDouble("4, 5, 6"));
  EXPECT_EQ((true, false, false), EvaluateBool("true, false, false"));
#pragma GCC diagnostic pop
}
