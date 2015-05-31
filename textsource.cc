#include "textsource.h"
#include "exception.h"

#include <iostream>

TextSource::TextSource(std::istream& in_stream, bool skip)
    : in(in_stream),
      current_char(0),
      skipping_comments(skip),
      line_number(1),
      column_number(0)
{
  consume();
}

void TextSource::consume() {
  // Read the next character
  if (in.eof()) {
    current_char = -1;
  } else {
    next();
  }

  // If we're not skipping comments, or in any case if this is
  // the last character, then we're done
  if (!skipping_comments || in.eof()) return;
  if (current_char == '/') {
    if (in.peek() == '/') {
      // Eat a comment to the end of the line
      while (current_char != '\n' && !in.eof()) {
        next();
      }
      current_char = '\n';  // Treat the entire comment as whitespace

    } else if (in.peek() == '*') {
      // Eat a comment to the '*/' mark
      bool star = false;  // tiny state machine; I either saw '*' or not
      next();  // skip the asterisk

      while (true) {
        if (in.eof()) throw Exception("Unterminated comment");

        next();
        if (star && current_char == '/') {
          // End of the comment. Treat the entire comment as whitespace.
          current_char = ' ';
          break;
        } else {
          star = current_char == '*';
        }
      }
    }
  }
}

bool TextSource::eof() const { return current_char == -1; }

void TextSource::skipComments(bool flag) {
  skipping_comments = flag;
}

void TextSource::next() {
  if (current_char == '\n') {
    ++line_number;
    column_number = 0;
  }

  current_char = in.get();
  ++column_number;
}
