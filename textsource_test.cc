#include "textsource.h"
#include "exception.h"

#include <iostream>
#include <sstream>
#include <string>

#include "gtest/gtest.h"

// Return the sequence of characters returned by the source.
std::string Consume(const std::string& text) {
  std::istringstream s(text);
  TextSource source(s);
  std::string result;

  while (!source.eof()) {
    result.append(1, source.current());
    source.consume();
  }

  return result;
}

TEST(TextSourceTest, Comments) {
  EXPECT_EQ(" ", Consume("/**/"));
  EXPECT_EQ(" ", Consume("/*/*/"));
  EXPECT_EQ("\n", Consume("// foo bar"));
  EXPECT_EQ("a b", Consume("a/* */b"));
  EXPECT_EQ("a b", Consume("a/* foo bar baz \n\nz*/b"));
  EXPECT_EQ("a b", Consume("a/* foo // bar\n*/b"));
  EXPECT_EQ("a\n", Consume("a// */b"));
  EXPECT_EQ("\n\n", Consume("// comment\n// foo"));
}

// Verifies that all characters are valid. As it turns out, the sequence of
// ASCII characters doesn't happen to form a comment mark (// or /*).
TEST(TextSourceTest, Characters) {
  std::string text;
  for (int i = 0; i < 256; i++) {
    text.append(1, i);
  }
  ASSERT_EQ(256, text.size());

  EXPECT_EQ(text, Consume(text));
  text = text + text;
  EXPECT_EQ(text, Consume(text));
}

TEST(TextSourceTest, Errors) {
  EXPECT_THROW(Consume("/*/"), Exception);
}

TEST(TextSourceTest, Position) {
  std::istringstream in("1/* */2\n// foo\n3\n");
  TextSource source(in);

  EXPECT_FALSE(source.eof());
  EXPECT_EQ('1', source.current());
  EXPECT_EQ(1, source.getLineNumber());
  EXPECT_EQ(1, source.getColumnNumber());
  source.consume();

  EXPECT_FALSE(source.eof());
  EXPECT_EQ(' ', source.current());
  EXPECT_EQ(1, source.getLineNumber());
  EXPECT_EQ(6, source.getColumnNumber());
  source.consume();

  EXPECT_FALSE(source.eof());
  EXPECT_EQ('2', source.current());
  EXPECT_EQ(1, source.getLineNumber());
  EXPECT_EQ(7, source.getColumnNumber());
  source.consume();

  EXPECT_FALSE(source.eof());
  EXPECT_EQ('\n', source.current());
  EXPECT_EQ(1, source.getLineNumber());
  EXPECT_EQ(8, source.getColumnNumber());
  source.consume();

  EXPECT_FALSE(source.eof());
  EXPECT_EQ('\n', source.current());
  EXPECT_EQ(2, source.getLineNumber());
  EXPECT_EQ(7, source.getColumnNumber());
  source.consume();

  EXPECT_FALSE(source.eof());
  EXPECT_EQ('3', source.current());
  EXPECT_EQ(3, source.getLineNumber());
  EXPECT_EQ(1, source.getColumnNumber());
  source.consume();

  EXPECT_FALSE(source.eof());
  EXPECT_EQ('\n', source.current());
  EXPECT_EQ(3, source.getLineNumber());
  EXPECT_EQ(2, source.getColumnNumber());
  source.consume();

  EXPECT_TRUE(source.eof());
}
