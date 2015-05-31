
#include "tokenizer.h"
#include "exception.h"

#include <ctype.h>
#include <sstream>

using namespace std;

/*
  Operators in precedence order, in C:

  ( ) [ ] -> .
  ! ~ ++ -- + - * & (type) sizeof
  * / %
  + -
  << >>
  < <= > >=
  == !=
  &
  ^
  |
  &&
  ||
  ?:
  = += -= *= /= %= &= ^= |= <<= >>=
  ,
*/

namespace {

static const char* const OPERATORS[] = {
    "^",
    "^=",
    "~",
    "<",
    "<<",
    "<<=",
    "<=",
    "=",
    "==",
    ">",
    ">>",
    ">>=",
    ">=",
    "|",
    "|=",
    "||",
    "-",
    "-=",
    "->",
    "--",
    ",",
    "!",
    "!=",
    "?",
    ":",
    "/",
    "/=",
    ".",
    "(",
    ")",
    "[",
    "]",
    "*",
    "*=",
    "&",
    "&=",
    "&&",
    "%",
    "%=",
    "+",
    "+=",
    "++",

    // the following aren't C operators, but symbols in the language

    ";",
    "{",
    "}",
    0
};

int findOp(const string& text) {
  for (int i = 0; OPERATORS[i] != 0; i++) {
    if (text == OPERATORS[i]) return i;
  }

  return -1;
}

int hexvalue(char c) {
  c = tolower(c);
  if ((c >= '0') && (c <= '9')) {
    return c - '0';
  } else {
    return (c - 'a') + 10;
  }
}

bool isoctal(char c) {
  return ((c >= '0') && (c <= '7'));
}

}  // namespace

Tokenizer::Tokenizer(TextSource* inSource) : source(inSource) {}

bool Tokenizer::next() {
  token.clear();

  skipWhiteSpace();
  if (source->eof()) return false;

  current_line = source->getLineNumber();
  current_column = source->getColumnNumber();

  if ((source->current() == '"') || (source->current() == '\'')) {
    token_type = TOK_STRING;
    readString(source->current());
    return true;
  } else if (isdigit(source->current())) {
    token_type = TOK_NUMBER;
    readNumber();
    return true;
  } else if (isalpha(source->current()) || (source->current() == '_')) {
    token_type = TOK_KEYWORD;
    readKeyword();
    return true;
  } else {
    token_type = TOK_OPERATOR;

    // Read an operator. This just appends characters until the
    // symbol's no longer in the OPERATORS list. That means that
    // all prefixes of multi-character operators (like '<<=') must
    // also be operators (in this case, '<' and '<<').
    while (!source->eof()) {
      token.append(1, source->current());

      int pos = findOp(token);
      if (pos >= 0) {
        source->consume();
        if (isspace(source->current())) break;
      } else {
        token.resize(token.size()-1);
        if (token.empty()) {
          throwError("Bad character '" + string(1,source->current()) + "'");
        }
        break;
      }
    }

    return !token.empty();
  }
}

void Tokenizer::skipWhiteSpace() {
  while ((source->current() != -1) && isspace(source->current())) {
    source->consume();
  }
}

class ResetFlag {
public:
  ResetFlag(TextSource* src) : source(src) {
    source->skipComments(false);
  }
  ~ResetFlag() { source->skipComments(true); }
private:
  TextSource* source;
};

void Tokenizer::readString(int delim) {
  ResetFlag resetter(source.get());

  token.clear();
  source->consume();

  while (true) {
    if (source->current() == -1) {
      throwError("Unterminated string");
      return;
    }

    if (source->current() == delim) {
      source->consume();
      return;
    }

    if (source->current() == '\\') {

      source->consume();

      if (isoctal(source->current())) {
        int result = source->current() - '0';
        source->consume();
        if (isoctal(source->current())) {
          result = result * 8 + (source->current() - '0');
          source->consume();
          if (isoctal(source->current())) {
            result = result * 8 + (source->current() - '0');
            source->consume();
          }
        }

        if (result > 255) {
          throwError("Octal escape sequence out of range");
        }

        token.append(1, result);

      } else if (source->current() == 'x') {

        source->consume();
        if (!isxdigit(source->current())) {
          throwError("Expecting a hex digit after \\x");
        }
        int result = hexvalue(source->current());
        source->consume();
        if (isxdigit(source->current())) {
          result = result * 16 + hexvalue(source->current());
          source->consume();
        }

        token.append(1, result);

      } else {
        switch (source->current()) {
          case 'a': token.append("\a"); break;
          case 'b': token.append("\b"); break;
          case 'f': token.append("\f"); break;
          case 'n': token.append("\n"); break;
          case 'r': token.append("\r"); break;
          case 't': token.append("\t"); break;
          case 'v': token.append("\v"); break;
          case '\\':
          case '\?':
          case '\'':
          case '\"':
            {
              token.append(1, source->current());
              break;
            }

          default:
            throwError("Unknown escape character '" +
                       string(1,source->current()) + "'");
        }

        source->consume();
      }
    } else {
      token.append(1, source->current());
      source->consume();
    }
  }
}

void Tokenizer::readNumber() {
  token.clear();

  if (source->current() == '0') {
    source->consume();
    if (tolower(source->current()) == 'x') {
      // hex
      source->consume();
      token = "0x";
      while (isxdigit(source->current())) {
        token.append(1, source->current());
        source->consume();
      }
      if (token == "0x") {
        throwError("No valid hex digits after 0x");
      }
    } else {
      // octal
      token = "0";
      while (isdigit(source->current())) {
        if (source->current() >= '8') {
          throwError("Digit out of range in octal constant");
        }
        token.append(1, source->current());
        source->consume();
      }
    }
  } else {
    while (isdigit(source->current())) {
      token.append(1, source->current());
      source->consume();
    }

    if (source->current() == '.') {
      token.append(1, source->current());
      source->consume();
    }

    while (isdigit(source->current())) {
      token.append(1, source->current());
      source->consume();
    }
  }
}

void Tokenizer::readKeyword() {
  token.clear();

  while (isalnum(source->current()) || source->current() == '_') {
    token.append(1, source->current());
    source->consume();
  }
}

void Tokenizer::throwError(const char* msg) {
  ostringstream out;
  out << msg << " (line " << source->getLineNumber()
      << ", column " << source->getColumnNumber() << ")";

  throw Exception(out.str());
}

void Tokenizer::throwError(const string& msg) {
  throwError(msg.c_str());
}
