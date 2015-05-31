#include "textsource.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

class Tokenizer {
public:
  enum TokenType {
    TOK_STRING,
    TOK_NUMBER,
    TOK_KEYWORD,
    TOK_OPERATOR
  };

  // Takes ownership of the TextSource
  Tokenizer(TextSource*);

  // Move on to the next (or first) token
  bool next();

  bool eof() const { return source->eof() && token.empty(); }

  // These are valid only after successful calls to 'next()'
  TokenType getTokenType() const { return token_type; }
  const std::string& getTokenText() const { return token; }

  int getLineNumber() const { return current_line; }
  int getColumnNumber() const { return current_column; }

private:
  void skipWhiteSpace();
  void readString(int delim);
  void readNumber();
  void readKeyword();

  void throwError(const char* msg);
  void throwError(const std::string& msg);

  std::unique_ptr<TextSource> source;

  std::string token;
  TokenType token_type;

  int current_column;
  int current_line;
};
