#include <iostream>
#include <sstream>
#include <string>

#include "exception.h"
#include "expression.h"
#include "textsource.h"
#include "tokenizer.h"

using namespace std;

int main(int argc, char* argv[]) {
  try {
    while (cin.good()) {
      std::string line;
      getline(cin, line);
      if (line.empty()) break;

      std::istringstream in(line);
      Tokenizer tokenizer(new TextSource(in));
      tokenizer.next();

      Expression* e = Expression::compile(tokenizer);
      POSTCONDITION(e != 0);

      e->print(cout);
      cout << " => ";
      ExecutionContext exe;
      Expression::Value v = e->evaluate(exe);

      cout << v << endl;
    }

    return 0;

  } catch (const Exception& e) {
    cerr << e.what() << endl;
  }

  return 1;
}
