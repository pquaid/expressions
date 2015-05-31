#if !defined EXPRESSION_H
#define      EXPRESSION_H

#include <iosfwd>
#include <string>
#include <vector>

class Tokenizer;

class ExecutionContext {
};

class Expression {
protected:
  Expression();

public:
  virtual ~Expression();

  enum Type {
    TYPE_UNKNOWN,
    TYPE_STRING,
    TYPE_NUMBER,
    TYPE_BOOL
  };

  enum Operator {
    OP_NOT,
    OP_BITNOT, // eg ~
    OP_NEGATIVE,
    OP_POSITIVE,  // Unary +
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MOD,
    OP_PLUS,
    OP_MINUS,
    OP_SHIFTLEFT,
    OP_SHIFTRIGHT,
    OP_LESS,
    OP_LESSEQ,
    OP_GREATER,
    OP_GREATEREQ,
    OP_EQUAL,
    OP_NOTEQUAL,
    OP_AND,
    OP_XOR,
    OP_OR,
    OP_ANDAND,
    OP_OROR,
    OP_TERNARY,    // ?:
    OP_ASSIGNMENT,
    OP_PLUSEQ,     // +=
    OP_MINUSEQ,    // -=
    OP_MULTIPLYEQ, // *=
    OP_DIVIDEEQ,   // /=
    OP_MODEQ,      // %=
    OP_ANDEQ,      // &=
    OP_XOREQ,      // ^=
    OP_OREQ,       // |=
    OP_LEFTEQ,     // <<=
    OP_RIGHTEQ,    // >>=
    OP_COMMA
  };

  struct Value {
    std::string stringValue;
    double      numberValue;
    bool        boolValue;
    Type        type;

    std::string asString() const;
    double asNumber() const;
    bool asBool() const;
  };

  virtual Value evaluate(ExecutionContext&) const = 0;
  
  virtual void print(std::ostream&) const = 0;

  static Expression* compile(Tokenizer&);

  static Operator string2operator(const std::string& text);
};

class ConstantExpression : public Expression {
public:
  ConstantExpression(const Value&);
  ConstantExpression(const std::string& s);
  ConstantExpression(bool b);
  ConstantExpression(double n);

  Value evaluate(ExecutionContext&) const override;
  void print(std::ostream&) const override;

  void set(double);
  void set(const std::string&);
  void set(bool);

  const std::string & getString() const;
  double getNumber() const;
  bool getBool() const;
  Type getType() const { return value.type; }

private:
    Value value;
};

class UnaryOperator : public Expression {
public:
  UnaryOperator(Operator, Expression* child);
  ~UnaryOperator() override;

  Value evaluate(ExecutionContext &) const override;
  void print(std::ostream &) const override;

private:
  Operator op;
  Expression* child;
};

class BinaryOperator : public Expression {
public:
  BinaryOperator(Operator op, Expression* left, Expression* right);
  ~BinaryOperator() override;

  Value evaluate(ExecutionContext &) const override;
  void print(std::ostream &) const override;

private:
  Operator op;
  Expression* left;
  Expression* right;
};

class TernaryOperator : public Expression {
public:
  TernaryOperator(Expression* test, Expression* pos, Expression* neg);
  ~TernaryOperator() override;

  Value evaluate(ExecutionContext&) const override;
  void print(std::ostream&) const override;

private:
  Expression* test;
  Expression* positive;
  Expression* negative;
};

class SequenceExpression : public Expression {
public:
  SequenceExpression();
  ~SequenceExpression() override;

  int getCount() const { return subs.size(); }
  const Expression* getSub(int i) const;

  void append(Expression*);

  Value evaluate(ExecutionContext& e) const override;
  void print(std::ostream&) const override;

private:
  std::vector<Expression*> subs;
};

// A couple handy printing operators
std::ostream& operator<<(std::ostream&, const Expression::Value&);
std::ostream& operator<<(std::ostream&, const Expression&);

#endif
