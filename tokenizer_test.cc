#include "tokenizer.h"

#include <iostream>
#include <vector>
#include <sstream>

#include "exception.h"

#include "gtest/gtest.h"

TEST(TokenizerTest, Comments) {
  {
    std::istringstream in("");
    Tokenizer tok(new TextSource(in));

    EXPECT_FALSE(tok.next());
    EXPECT_TRUE(tok.eof());
  }
  {
    std::istringstream in(" ");
    Tokenizer tok(new TextSource(in));

    EXPECT_FALSE(tok.next());
    EXPECT_TRUE(tok.eof());
  }
  {
    std::istringstream in(" \n ");
    Tokenizer tok(new TextSource(in));

    EXPECT_FALSE(tok.next());
    EXPECT_TRUE(tok.eof());
  }
  {
    std::istringstream in("/**/");
    Tokenizer tok(new TextSource(in));

    EXPECT_FALSE(tok.next());
    EXPECT_TRUE(tok.eof());
  }
  {
    std::istringstream in("1 /* */, // 4\n'abc' ");
    Tokenizer tok(new TextSource(in));

    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_NUMBER, tok.getTokenType());
    EXPECT_EQ("1", tok.getTokenText());
    EXPECT_EQ(1, tok.getLineNumber());
    EXPECT_EQ(1, tok.getColumnNumber());

    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_OPERATOR, tok.getTokenType());
    EXPECT_EQ(",", tok.getTokenText());
    EXPECT_EQ(1, tok.getLineNumber());
    EXPECT_EQ(8, tok.getColumnNumber());

    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_STRING, tok.getTokenType());
    EXPECT_EQ("abc", tok.getTokenText());
    EXPECT_EQ(2, tok.getLineNumber());
    EXPECT_EQ(1, tok.getColumnNumber());

    EXPECT_FALSE(tok.next());
  }
}

TEST(TokenizerTest, Operators) {
  {
    std::istringstream in("<=");
    Tokenizer tok(new TextSource(in));
    ASSERT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_OPERATOR, tok.getTokenType());
    EXPECT_EQ("<=", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in("=>");
    Tokenizer tok(new TextSource(in));
    ASSERT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_OPERATOR, tok.getTokenType());
    EXPECT_EQ("=", tok.getTokenText());
    EXPECT_EQ(1, tok.getColumnNumber());
    EXPECT_TRUE(tok.next());

    EXPECT_EQ(Tokenizer::TOK_OPERATOR, tok.getTokenType());
    EXPECT_EQ(">", tok.getTokenText());
    EXPECT_EQ(2, tok.getColumnNumber());
    EXPECT_FALSE(tok.next());
  }
  // The tokenizer always generates the longest valid tokens.
  {
    std::istringstream in(">");
    Tokenizer tok(new TextSource(in));
    ASSERT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_OPERATOR, tok.getTokenType());
    EXPECT_EQ(">", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in(">>");
    Tokenizer tok(new TextSource(in));
    ASSERT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_OPERATOR, tok.getTokenType());
    EXPECT_EQ(">>", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in(">>=");
    Tokenizer tok(new TextSource(in));
    ASSERT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_OPERATOR, tok.getTokenType());
    EXPECT_EQ(">>=", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in(">>===");
    Tokenizer tok(new TextSource(in));
    ASSERT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_OPERATOR, tok.getTokenType());
    EXPECT_EQ(">>=", tok.getTokenText());
    EXPECT_TRUE(tok.next());

    EXPECT_EQ(Tokenizer::TOK_OPERATOR, tok.getTokenType());
    EXPECT_EQ("==", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }

  // Error cases
  {
    std::istringstream in("$");
    Tokenizer tok(new TextSource(in));
    EXPECT_THROW(tok.next(), Exception);
  }
}

TEST(TokenizerTest, Strings) {
  {
    std::istringstream in(" \"foo bar\" ");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_STRING, tok.getTokenType());
    EXPECT_EQ("foo bar", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in(" \"foo\\\"bar\" ");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_STRING, tok.getTokenType());
    EXPECT_EQ("foo\"bar", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in(" 'foo\\'bar' ");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_STRING, tok.getTokenType());
    EXPECT_EQ("foo'bar", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in("'foo /* bar */'");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_STRING, tok.getTokenType());
    EXPECT_EQ("foo /* bar */", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in("'\\x20'");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_STRING, tok.getTokenType());
    EXPECT_EQ(" ", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in("'\\40'");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_STRING, tok.getTokenType());
    EXPECT_EQ(" ", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }

  // Error cases
  {
    std::istringstream in("'foo");
    Tokenizer tok(new TextSource(in));
    EXPECT_THROW(tok.next(), Exception);
  }
  {
    std::istringstream in("'\\z'");
    Tokenizer tok(new TextSource(in));
    EXPECT_THROW(tok.next(), Exception);
  }
  {
    std::istringstream in("'\\xz'");
    Tokenizer tok(new TextSource(in));
    EXPECT_THROW(tok.next(), Exception);
  }
  {
    std::istringstream in("'\\9'");
    Tokenizer tok(new TextSource(in));
    EXPECT_THROW(tok.next(), Exception);
  }
}

TEST(TokenizerTest, Numbers) {
  {
    std::istringstream in(" 123 ");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_NUMBER, tok.getTokenType());
    EXPECT_EQ("123", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in(" 123.5 ");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_NUMBER, tok.getTokenType());
    EXPECT_EQ("123.5", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in(" 123.5.5 ");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_NUMBER, tok.getTokenType());
    EXPECT_EQ("123.5", tok.getTokenText());
    EXPECT_TRUE(tok.next());
  }
  {
    std::istringstream in(" 0123 ");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_NUMBER, tok.getTokenType());
    EXPECT_EQ("0123", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in(" 0x12aA ");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_NUMBER, tok.getTokenType());
    EXPECT_EQ("0x12aA", tok.getTokenText());
    EXPECT_FALSE(tok.next());
  }
  {
    std::istringstream in(" 0x12aA.4 ");
    Tokenizer tok(new TextSource(in));
    EXPECT_TRUE(tok.next());
    EXPECT_EQ(Tokenizer::TOK_NUMBER, tok.getTokenType());
    EXPECT_EQ("0x12aA", tok.getTokenText());
    EXPECT_TRUE(tok.next());
  }

  // Error cases
  {
    std::istringstream in(" 09 ");
    Tokenizer tok(new TextSource(in));
    EXPECT_THROW(tok.next(), Exception);
  }
  {
    std::istringstream in(" 0xZ ");
    Tokenizer tok(new TextSource(in));
    EXPECT_THROW(tok.next(), Exception);
  }
}

TEST(TokenizerTest, Keywords) {
  std::istringstream in(" foo_bar_ ");
  Tokenizer tok(new TextSource(in));
  EXPECT_TRUE(tok.next());
  EXPECT_EQ(Tokenizer::TOK_KEYWORD, tok.getTokenType());
  EXPECT_EQ("foo_bar_", tok.getTokenText());
  EXPECT_FALSE(tok.next());
}
