#include "expression.h"
#include "exception.h"
#include "textsource.h"
#include "tokenizer.h"

#include <iostream>
#include <sstream>
#include <stdlib.h>

using namespace std;

Expression::Expression() {}
Expression::~Expression() {}


struct OperatorInfo {
  Expression::Operator op;
  const char* text;
  int level;
};

static OperatorInfo opInfo[] =
{
  { Expression::OP_NOT,        "!",    2 },
  { Expression::OP_BITNOT,     "~",    2 },
  { Expression::OP_NEGATIVE,   "-",    2 },
  { Expression::OP_POSITIVE,   "+",    2 },
  { Expression::OP_MULTIPLY,   "*",    3 },
  { Expression::OP_DIVIDE,     "/",    3 },
  { Expression::OP_MOD,        "%",    3 },
  { Expression::OP_PLUS,       "+",    4 },
  { Expression::OP_MINUS,      "-",    4 },
  { Expression::OP_SHIFTLEFT,  "<<",   5 },
  { Expression::OP_SHIFTRIGHT, ">>",   5 },
  { Expression::OP_LESS,       "<",    6 },
  { Expression::OP_LESSEQ,     "<=",   6 },
  { Expression::OP_GREATER,    ">",    6 },
  { Expression::OP_GREATEREQ,  ">=",   6 },
  { Expression::OP_EQUAL,      "==",   7 },
  { Expression::OP_NOTEQUAL,   "!=",   7 },
  { Expression::OP_AND,        "&",    8 },
  { Expression::OP_XOR,        "^",    9 },
  { Expression::OP_OR,         "|",   10 },
  { Expression::OP_ANDAND,     "&&",  11 },
  { Expression::OP_OROR,       "||",  12 },
  { Expression::OP_TERNARY,    "?",   13 },
  { Expression::OP_ASSIGNMENT, "=",   14 },
  { Expression::OP_PLUSEQ,     "+=",  14 },
  { Expression::OP_MINUSEQ,    "-=",  14 },
  { Expression::OP_MULTIPLYEQ, "*=",  14 },
  { Expression::OP_DIVIDEEQ,   "/=",  14 },
  { Expression::OP_MODEQ,      "%=",  14 },
  { Expression::OP_ANDEQ,      "&=",  14 },
  { Expression::OP_XOREQ,      "^=",  14 },
  { Expression::OP_OREQ,       "|=",  14 },
  { Expression::OP_LEFTEQ,     "<<=", 14 },
  { Expression::OP_RIGHTEQ,    ">>=", 14 },
  { Expression::OP_COMMA,      ",",   15 },
  { Expression::OP_NOT, 0, 0 }
};

Expression::Operator Expression::string2operator(const std::string& s) {
  for (int i = 0; opInfo[i].text != 0; i++) {
    if (s == opInfo[i].text) return opInfo[i].op;
  }

  throw Exception("Unknown operator " + s);
}

namespace {

const char* operator2string(Expression::Operator op) {
  for (int i = 0; opInfo[i].text != 0; i++) {
    if (op == opInfo[i].op) return opInfo[i].text;
  }
  return "(unknown op)";
}

double toNumber(const string & s) {
  char* endptr = nullptr;
  const double result = strtod(s.c_str(), &endptr);
  if (endptr == s.c_str() || endptr == nullptr || *endptr != '\0') {
    throw Exception("Invalid conversion to number: '" + s + "'");
  }
  return result;
}

string toString(const double v) {
  ostringstream o;
  o << v;
  return o.str();
}

Expression::Type upcastType(Expression::Type left, Expression::Type right) {
  if ((left == Expression::TYPE_UNKNOWN) ||
      (right == Expression::TYPE_UNKNOWN)) {
    return Expression::TYPE_UNKNOWN;
  } else if ((left == Expression::TYPE_STRING) ||
             (right == Expression::TYPE_STRING)) {
    return Expression::TYPE_STRING;
  } else if ((left == Expression::TYPE_NUMBER) ||
             (right == Expression::TYPE_NUMBER)) {
    return Expression::TYPE_NUMBER;
  } else {
    return Expression::TYPE_BOOL;
  }
}

double toNumber(const Expression::Value& v) {
  if (v.type == Expression::TYPE_NUMBER) {
    return v.numberValue;
  } else if (v.type == Expression::TYPE_STRING) {
    return toNumber(v.stringValue);
  } else {
    return (v.boolValue ? 1 : 0);
  }
}

int32_t toInt(const Expression::Value& v) {
  if (v.type == Expression::TYPE_NUMBER) {
    return (int32_t) v.numberValue;
  } else if (v.type == Expression::TYPE_STRING) {
    return (int32_t) toNumber(v.stringValue);
  } else {
    return (v.boolValue ? 1 : 0);
  }
}

string toString(const Expression::Value & v) {
  if (v.type == Expression::TYPE_STRING) {
    return v.stringValue;
  } else if (v.type == Expression::TYPE_NUMBER) {
    return toString(v.numberValue);
  } else {
    return (v.boolValue ? "true" : "false");
  }
}

bool toBool(const Expression::Value & v) {
  if (v.type == Expression::TYPE_BOOL) {
    return v.boolValue;
  } else if (v.type == Expression::TYPE_STRING) {
    if (v.stringValue == "true") {
      return true;
    } else if (v.stringValue == "false") {
      return false;
    } else {
      return !v.stringValue.empty();
    }
  } else {
    return (v.numberValue != 0);
  }
}

Expression::Value upcast(Expression::Value v, Expression::Type toType) {
  if (v.type == toType) {
    return v;
  }
  Expression::Value result;
  result.type = toType;

  if (toType == Expression::TYPE_STRING) {
    if (v.type == Expression::TYPE_NUMBER) {
      result.stringValue = toString(v.numberValue);
    } else {
      result.stringValue = (v.boolValue ? "true" : "false");
    }
  } else if (toType == Expression::TYPE_NUMBER) {
    if (v.type == Expression::TYPE_STRING) {
      v.numberValue = toNumber(v.stringValue);
    } else {
      v.numberValue = (v.boolValue ? 1 : 0);
    }
  } else {
    if (v.type == Expression::TYPE_STRING) {
      v.boolValue = !v.stringValue.empty();
    } else {
      v.boolValue = v.numberValue != 0;
    }
  }

  return result;
}

void expect(Tokenizer& tok, Tokenizer::TokenType type, const char* text) {
  if ((tok.getTokenType() == type) && (tok.getTokenText() == text)) {
    tok.next();
  } else {
    ostringstream out;
    out << "Syntax error. Expecting \"" << text << "\" at line "
        << tok.getLineNumber() << ", column "
        << tok.getColumnNumber();
    
    throw Exception(out.str());
  }
}

Expression* compileConstant(Tokenizer & tok) {
  if (tok.eof()) {
    throw Exception("Syntax error: unexpected end of input");
  }

  if (tok.getTokenType() == Tokenizer::TOK_STRING) {
    auto* result = new ConstantExpression(tok.getTokenText());
    tok.next();
    return result;
  } else if (tok.getTokenType() == Tokenizer::TOK_NUMBER) {
    auto* result = new ConstantExpression(toNumber(tok.getTokenText()));
    tok.next();
    return result;
  } else if (tok.getTokenType() == Tokenizer::TOK_KEYWORD) {
    bool val;

    if (tok.getTokenText() == "true") {
      val = true;
    } else if (tok.getTokenText() == "false") {
      val = false;
    } else {
      throw Exception("Unexpected keyword: " + tok.getTokenText());
    }

    tok.next();
    return new ConstantExpression(val);
  }

  throw Exception("Unknown token type");
}

// Forward declaration.
Expression* compileSequence(Tokenizer& tok);

Expression* compileBrackets(Tokenizer& tok) {
  if (tok.getTokenType() != Tokenizer::TOK_OPERATOR) {
    return compileConstant(tok);
  }

  Expression* expr = 0;
  if (tok.getTokenText() == "(") {
    tok.next();
    expr = compileSequence(tok);
    expect(tok, Tokenizer::TOK_OPERATOR, ")");
  } else if (tok.getTokenText() == "[") {
    tok.next();
    expr = compileSequence(tok);
    expect(tok, Tokenizer::TOK_OPERATOR, "]");
  } else if (tok.getTokenText() == "{") {
    tok.next();
    expr = compileSequence(tok);
    expect(tok, Tokenizer::TOK_OPERATOR, "}");
  } else {
    throw Exception("Unexpected operator: " + tok.getTokenText());
  }

  return expr;
}

Expression* compileUnary(Tokenizer& tok) {
  if (tok.getTokenType() == Tokenizer::TOK_OPERATOR) {
    int match = -1;
    for (int i = 0; (match < 0) && (opInfo[i].text != 0); i++) {
      if ((tok.getTokenText() == opInfo[i].text) &&
          (opInfo[i].level == 2)) {
        match = i;
      }
    }

    if (match >= 0) {
      tok.next();
      return new UnaryOperator(opInfo[match].op, compileUnary(tok));
    }
  }

  return compileBrackets(tok);
}

Expression* compileTernary(Tokenizer& tok, Expression* condition) {
  Expression* positive = compileSequence(tok);
  expect(tok, Tokenizer::TOK_OPERATOR, ":");
  Expression* negative = compileSequence(tok);
  return new TernaryOperator(condition, positive, negative);
}

// Parse a left-associative binary operator. Most of 'em, as it turns
// out. This is based on the table above. At level 2 and below, we get
// more specific because either the operators are unary, right associative
// or brackets.
Expression* compileLevel(Tokenizer& tok, int level) {
  if (level == 2) {
    return compileUnary(tok);
  }

  Expression* expr = compileLevel(tok, level - 1);

  // Process left-associative binary operators
  while (true) {
    if (tok.eof() || (tok.getTokenType() != Tokenizer::TOK_OPERATOR)) {
      return expr;
    }

    bool found = false;
    for (int i = 0; !found && (opInfo[i].text != 0); i++) {
      if ((opInfo[i].level == level) &&
          (tok.getTokenText() == opInfo[i].text)) {

        tok.next();
        if (opInfo[i].op == Expression::OP_TERNARY) {
          expr = compileTernary(tok, expr);
        } else {
          expr = new BinaryOperator(opInfo[i].op,
                                    expr,
                                    compileLevel(tok, level-1));
        }
        found = true;
      }
    }

    if (!found) break;
  }

  return expr;
}

Expression* compileSequence(Tokenizer& tok) {
  Expression* expr = compileLevel(tok, 14);
  SequenceExpression* seq = nullptr;

  while (!tok.eof() &&
         (tok.getTokenType() == Tokenizer::TOK_OPERATOR) &&
         (tok.getTokenText() == ",")) {
    if (seq == nullptr) {
      seq = new SequenceExpression();
      seq->append(expr);
    }

    tok.next();
    seq->append(compileLevel(tok, 14));
  }

  if (seq != nullptr) {
    return seq;
  } else {
    return expr;
  }
}

}  // namespace

std::string Expression::Value::asString() const {
  return toString(*this);
}

double Expression::Value::asNumber() const {
  return toNumber(*this);
}

bool Expression::Value::asBool() const {
  return toBool(*this);
}

Expression* Expression::compile(Tokenizer& tok) {
  std::unique_ptr<Expression> result(compileSequence(tok));
  if (!tok.eof()) {
    std::ostringstream out;
    out << "Extraneous text after expression at line " << tok.getLineNumber()
        << ", column " << tok.getColumnNumber();
    throw Exception(out.str());
  }
  return result.release();
}

ConstantExpression::ConstantExpression(const Value& v)
  : value(v)
{}

ConstantExpression::ConstantExpression(const std::string& s)
    : value({ s, 0, false, TYPE_STRING }) {}

ConstantExpression::ConstantExpression(double n)
    : value({ "", n, false, TYPE_NUMBER }) {}

ConstantExpression::ConstantExpression(bool b)
    : value({ "", 0, b, TYPE_BOOL }) {}

Expression::Value ConstantExpression::evaluate(ExecutionContext&) const {
  return value;
}

void ConstantExpression::print(ostream& out) const {
  out << value;
}

void ConstantExpression::set(double v) {
  value.numberValue = v;
  value.type = TYPE_NUMBER;
}

void ConstantExpression::set(const string & s) {
  value.stringValue = s;
  value.type = TYPE_STRING;
}

void ConstantExpression::set(bool f) {
  value.boolValue = f;
  value.type = TYPE_BOOL;
}

const string & ConstantExpression::getString() const {
  if (value.type != TYPE_STRING) {
    throw Exception("Invalid type; not a string");
  }
  return value.stringValue;
}

double ConstantExpression::getNumber() const {
  if (value.type != TYPE_NUMBER) {
    throw Exception("Invalid type; not a number");
  }
  return value.numberValue;
}

bool ConstantExpression::getBool() const {
  if (value.type != TYPE_BOOL) {
    throw Exception("Invalid type; not a Boolean");
  }
  return value.boolValue;
}

UnaryOperator::UnaryOperator(Expression::Operator inOp, Expression* inChild)
    : op(inOp), child(inChild)
{}

UnaryOperator::~UnaryOperator() {
  delete child;
}

Expression::Value UnaryOperator::evaluate(ExecutionContext& e) const {
  Expression::Value result = child->evaluate(e);
  if (op == OP_NOT) {
    result.boolValue = !result.asBool();
    result.type = TYPE_BOOL;
  } else if (op == OP_BITNOT) {
    if (result.type == TYPE_BOOL) {
      result.boolValue = !result.asBool();
    } else {
      int64_t i = result.asNumber();
      result.numberValue = ~i;
      result.type = TYPE_NUMBER;
    }
  } else if (op == OP_NEGATIVE) {
    result.numberValue = -result.asNumber();
    result.type = TYPE_NUMBER;
  } else if (op == OP_POSITIVE) {
    result.numberValue = result.asNumber();
    result.type = TYPE_NUMBER;
  } else {
    throw Exception("Unknown unary operator");
  }

  return result;
}

void UnaryOperator::print(ostream& out) const {
  out << operator2string(op);
  child->print(out);
}

BinaryOperator::BinaryOperator(Expression::Operator inOp,
                               Expression* inLeft,
                               Expression* inRight)
    : op(inOp), left(inLeft), right(inRight) {
}

BinaryOperator::~BinaryOperator() {
  delete left;
  delete right;
}

Expression::Value BinaryOperator::evaluate(ExecutionContext& e) const {
  // None of the assignment operators make sense, since we don't have
  // lvalues.
  switch (op) {
    case OP_ASSIGNMENT:
    case OP_PLUSEQ:
    case OP_MINUSEQ:
    case OP_MULTIPLYEQ:
    case OP_DIVIDEEQ:
    case OP_MODEQ:
    case OP_ANDEQ:
    case OP_XOREQ:
    case OP_OREQ:
    case OP_LEFTEQ:
    case OP_RIGHTEQ:
      throw Exception("Not implemented: " + string(operator2string(op)));
    default:
      ;
  }

  Value leftValue = left->evaluate(e);
  Value rightValue = right->evaluate(e);

  Value result;
  result.type = upcastType(leftValue.type, rightValue.type);
  if (result.type == TYPE_STRING) {
    leftValue.stringValue = toString(leftValue);
    rightValue.stringValue = toString(rightValue);

    switch (op) {
      case OP_PLUS:
        result.stringValue = leftValue.stringValue + rightValue.stringValue;
        break;
      case OP_LESS:
        result.boolValue = leftValue.stringValue < rightValue.stringValue;
        result.type = TYPE_BOOL;
        break;
      case OP_LESSEQ:
        result.boolValue = leftValue.stringValue <= rightValue.stringValue;
        result.type = TYPE_BOOL;
        break;
      case OP_GREATER:
        result.boolValue = leftValue.stringValue > rightValue.stringValue;
        result.type = TYPE_BOOL;
        break;
      case OP_GREATEREQ:
        result.boolValue = leftValue.stringValue >= rightValue.stringValue;
        result.type = TYPE_BOOL;
        break;
      case OP_EQUAL:
        result.boolValue = leftValue.stringValue == rightValue.stringValue;
        result.type = TYPE_BOOL;
        break;
      case OP_NOTEQUAL:
        result.boolValue = leftValue.stringValue != rightValue.stringValue;
        result.type = TYPE_BOOL;
        break;
      default:
        throw Exception(string("Invalid operation (") +
                        operator2string(op) + ") on strings");
    }

  } else if (result.type == TYPE_NUMBER) {
    leftValue.numberValue = toNumber(leftValue);
    rightValue.numberValue = toNumber(rightValue);

    switch (op) {
      case OP_MULTIPLY:
        result.numberValue = leftValue.numberValue * rightValue.numberValue;
        break;
      case OP_DIVIDE:
        result.numberValue = leftValue.numberValue / rightValue.numberValue;
        break;
      case OP_MOD:
        result.numberValue = toInt(leftValue) % toInt(rightValue);
        break;
      case OP_PLUS:
        result.numberValue = leftValue.numberValue + rightValue.numberValue;
        break;
      case OP_MINUS:
        result.numberValue = leftValue.numberValue - rightValue.numberValue;
        break;
      case OP_SHIFTLEFT:
        result.numberValue = toInt(leftValue) << toInt(rightValue);
        break;
      case OP_SHIFTRIGHT:
        result.numberValue = toInt(leftValue) >> toInt(rightValue);
        break;
      case OP_LESS:
        result.boolValue = leftValue.numberValue < rightValue.numberValue;
        result.type = TYPE_BOOL;
        break;
      case OP_LESSEQ:
        result.boolValue = leftValue.numberValue <= rightValue.numberValue;
        result.type = TYPE_BOOL;
        break;
      case OP_GREATER:
        result.boolValue = leftValue.numberValue > rightValue.numberValue;
        result.type = TYPE_BOOL;
        break;
      case OP_GREATEREQ:
        result.boolValue = leftValue.numberValue >= rightValue.numberValue;
        result.type = TYPE_BOOL;
        break;
      case OP_EQUAL:
        result.boolValue = leftValue.numberValue == rightValue.numberValue;
        result.type = TYPE_BOOL;
        break;
      case OP_NOTEQUAL:
        result.boolValue = leftValue.numberValue != rightValue.numberValue;
        result.type = TYPE_BOOL;
        break;
      case OP_AND:
        result.numberValue = toInt(leftValue) & toInt(rightValue);
        break;
      case OP_XOR:
        result.numberValue = toInt(leftValue) ^ toInt(rightValue);
        break;
      case OP_OR:
        result.numberValue = toInt(leftValue) | toInt(rightValue);
        break;
      case OP_ANDAND:
        result.boolValue = toBool(leftValue) && toBool(rightValue);
        result.type = TYPE_BOOL;
        break;
      case OP_OROR:
        result.boolValue = toBool(leftValue) || toBool(rightValue);
        result.type = TYPE_BOOL;
        break;
      default:
        throw Exception(string("Invalid operation (") +
                        operator2string(op) + ") on numbers");
    }

  } else if (result.type == TYPE_BOOL) {
    bool lval = toBool(leftValue);
    bool rval = toBool(rightValue);
    result.type = TYPE_BOOL;

    switch (op) {
      case OP_EQUAL:
        result.boolValue = lval == rval;
        break;
      case OP_NOTEQUAL:
        result.boolValue = lval != rval;
        break;
      case OP_ANDAND:
        result.boolValue = lval && rval;
        break;
      case OP_OROR:
        result.boolValue = lval || rval;
        break;
      default:
        throw Exception(string("Invalid operation (") +
                        operator2string(op) + ") on Boolean values");
    }

  } else if (result.type == TYPE_UNKNOWN) {
    // Unknown type -- we've given up trying to evaluate
  } else {
    ASSERTION(false);
  }

  return result;
}

void BinaryOperator::print(ostream& out) const {
  out << "(";
  left->print(out);
  out << operator2string(op);
  right->print(out);
  out << ")";
}

TernaryOperator::TernaryOperator(Expression* condition,
                                 Expression* pos,
                                 Expression* neg)
    : test(condition), positive(pos), negative(neg) {}

TernaryOperator::~TernaryOperator() {
  delete test;
  delete positive;
  delete negative;
}

Expression::Value TernaryOperator::evaluate(ExecutionContext& e) const {
  if (test->evaluate(e).asBool()) {
    return positive->evaluate(e);
  } else {
    return negative->evaluate(e);
  }
}

void TernaryOperator::print(ostream& out) const {
  out << "(";
  test->print(out);
  out << "?";
  positive->print(out);
  out << ":";
  negative->print(out);
  out << ")";
}

SequenceExpression::SequenceExpression() {}

SequenceExpression::~SequenceExpression() {
  for (auto* sub : subs) {
    delete sub;
  }
}

const Expression* SequenceExpression::getSub(int i) const {
  PRECONDITION(i < (int) subs.size());
  return subs[i];
}

void SequenceExpression::append(Expression* expr) {
  PRECONDITION(expr != 0);
  subs.push_back(expr);
}

Expression::Value SequenceExpression::evaluate(ExecutionContext & e) const {
  if (subs.empty()) {
    throw Exception("Attempt to execute an empty sequence");
  }

  Value v;
  for (const auto* sub : subs) {
    v = sub->evaluate(e);
  }
  return v;
}

void SequenceExpression::print(ostream& out) const {
  for (unsigned i = 0; i < subs.size(); i++) {
    if (i > 0) out << ",";
    subs[i]->print(out);
  }
}

ostream& operator<<(ostream& out, const Expression::Value& v) {
  if (v.type == Expression::TYPE_NUMBER) {
    out << v.numberValue;
  } else if (v.type == Expression::TYPE_STRING) {
    out << "\"" << v.stringValue << "\"";
  } else if (v.type == Expression::TYPE_BOOL) {
    out << (v.boolValue ? "true" : "false");
  } else {
    out << "(invalid value)";
  }

  return out;
}

ostream& operator<<(ostream& out, const Expression& e) {
  e.print(out);
  return out;
}

