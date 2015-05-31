#if !defined TEXTSOURCE_H
#define      TEXTSOURCE_H

#include <iosfwd>

// TextSource consumes an istream char by char, keeping track of line and
// column position and skipping over comments.
class TextSource {
public:
  TextSource(std::istream&, bool skip_comments = true);

  // Return the current character; valid only if !eof()
  char current() const { return current_char; }

  // Move on to the next character
  void consume();

  // Have we consumed everything?
  bool eof() const;

  // Get the position of 'current()' within the text
  int getLineNumber() const { return line_number; }
  int getColumnNumber() const { return column_number; }

  // Shall we eat code comments? We normally want to, but if the consumer
  // is reading a string, it temporarily turns off this processing.
  void skipComments(bool flag);
  bool getSkippingComments() const { return skipping_comments; }

private:
  // Sets current_char to the next character in the stream, and keeps
  // track of line/column counts.
  void next();

  std::istream& in;

  int current_char;
  bool skipping_comments;
  int line_number;
  int column_number;
};

#endif
