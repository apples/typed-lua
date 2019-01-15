#ifndef TYPEDLUA_NODE_HPP
#define TYPEDLUA_NODE_HPP

#include "compile_error.hpp"
#include "location.hpp"
#include "scope.hpp"
#include "type.hpp"

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace typedlua::ast {

class Node {
public:
    virtual ~Node() = default;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const;

    Location location;
};

inline std::ostream& operator<<(std::ostream& out, const Node& n) {
    n.dump(out);
    return out;
}

class NExpr : public Node {
public:
    virtual Type get_type(const Scope& scope) const = 0;

    virtual void check_expect(Scope& parent_scope, const Type& expected, std::vector<CompileError>& errors) const;
};

class NBlock : public Node {
public:
    std::vector<std::unique_ptr<Node>> children;
    bool scoped = false;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NType : public Node {
public:
    virtual void dump(std::ostream& out) const override final;

    virtual Type get_type(const Scope& scope) const = 0;
};

class NNameDecl : public Node {
public:
    NNameDecl() = default;
    NNameDecl(std::string name);
    NNameDecl(std::string name, std::unique_ptr<NType> type);
    std::string name;
    std::unique_ptr<NType> type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    Type get_type(const Scope& scope) const;
};

class NTypeName : public NType {
public:
    NTypeName() = default;
    NTypeName(std::string name);
    std::string name;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual Type get_type(const Scope& scope) const override;
};

class NTypeFunctionParam : public Node {
public:
    NTypeFunctionParam() = default;
    NTypeFunctionParam(std::string name, std::unique_ptr<NType> type);
    std::string name;
    std::unique_ptr<NType> type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NTypeFunction : public NType {
public:
    NTypeFunction() = default;
    NTypeFunction(std::vector<NTypeFunctionParam> p, std::unique_ptr<NType> r, bool v);
    std::vector<NNameDecl> generic_params;
    std::vector<NTypeFunctionParam> params;
    std::unique_ptr<NType> ret;
    bool is_variadic;
    mutable Type cached_type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual Type get_type(const Scope& parent_scope) const override;
};

class NTypeTuple : public NType {
public:
    NTypeTuple() = default;
    NTypeTuple(std::vector<NTypeFunctionParam> p, bool v);
    std::vector<NTypeFunctionParam> params;
    bool variadic;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual Type get_type(const Scope& scope) const override;
};

class NTypeSum : public NType {
public:
    NTypeSum() = default;
    NTypeSum(std::unique_ptr<NType> l, std::unique_ptr<NType> r);
    std::unique_ptr<NType> lhs;
    std::unique_ptr<NType> rhs;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual Type get_type(const Scope& scope) const override;
};

class NIndex : public Node {
public:
    NIndex() = default;
    NIndex(std::unique_ptr<NType> k, std::unique_ptr<NType> v);
    std::unique_ptr<NType> key;
    std::unique_ptr<NType> val;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    KeyValPair get_kvp(const Scope& scope) const;
};

class NIndexList : public Node {
public:
    NIndexList() = default;
    std::vector<std::unique_ptr<NIndex>> indexes;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
    std::vector<KeyValPair> get_types(const Scope& scope) const;
};

class NFieldDecl : public Node {
public:
    NFieldDecl() = default;
    NFieldDecl(std::string n, std::unique_ptr<NType> t);
    std::string name;
    std::unique_ptr<NType> type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NFieldDeclList : public Node {
public:
    NFieldDeclList() = default;
    std::vector<std::unique_ptr<NFieldDecl>> fields;
    mutable FieldMap cached_fields;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    FieldMap get_types(const Scope& scope) const;
};

class NTypeTable : public NType {
public:
    NTypeTable() = default;
    NTypeTable(std::unique_ptr<NIndexList> i, std::unique_ptr<NFieldDeclList> f);
    std::unique_ptr<NIndexList> indexlist;
    std::unique_ptr<NFieldDeclList> fieldlist;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual Type get_type(const Scope& scope) const override;
};

class NTypeLiteralBoolean : public NType {
public:
    NTypeLiteralBoolean() = default;
    NTypeLiteralBoolean(bool v);
    bool value;

    virtual Type get_type(const Scope& scope) const override;
};

class NTypeLiteralNumber : public NType {
public:
    NTypeLiteralNumber() = default;
    NTypeLiteralNumber(const std::string& s);
    NumberRep value;

    virtual Type get_type(const Scope& scope) const override;
};

class NTypeLiteralString : public NType {
public:
    NTypeLiteralString() = default;
    NTypeLiteralString(std::string_view s);
    std::string value;

    virtual Type get_type(const Scope& scope) const override;
};

class NInterface : public Node {
public:
    NInterface() = default;
    NInterface(std::string n, std::unique_ptr<NType> t);
    std::string name;
    std::unique_ptr<NType> type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NIdent : public NExpr {
public:
    NIdent() = default;
    NIdent(std::string v);
    std::string name;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void check_expect(Scope& parent_scope, const Type& expected, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;

private:
    void fail_common(Scope& parent_scope, std::vector<CompileError>& errors) const;
};

class NSubscript : public NExpr {
public:
    NSubscript() = default;
    NSubscript(std::unique_ptr<NExpr> p, std::unique_ptr<NExpr> s);
    std::unique_ptr<NExpr> prefix;
    std::unique_ptr<NExpr> subscript;
    mutable std::optional<Type> cached_type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void check_expect(Scope& parent_scope, const Type& expected, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;

private:
    void check_common(const Type& prefixtype, const Type& keytype, Scope& parent_scope, std::vector<CompileError>& errors) const;
};

class NTableAccess : public NExpr {
public:
    NTableAccess() = default;
    NTableAccess(std::unique_ptr<NExpr> p, std::string n);
    std::unique_ptr<NExpr> prefix;
    std::string name;
    mutable std::optional<Type> cached_type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void check_expect(Scope& parent_scope, const Type& expected, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;

private:
    void check_common(const Type& prefixtype, Scope& parent_scope, std::vector<CompileError>& errors) const;
};

class NArgSeq : public Node {
public:
    NArgSeq() = default;
    std::vector<std::unique_ptr<NExpr>> args;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NFunctionCall : public NExpr {
public:
    NFunctionCall() = default;
    NFunctionCall(std::unique_ptr<NExpr> p, std::unique_ptr<NArgSeq> a);
    std::unique_ptr<NExpr> prefix;
    std::unique_ptr<NArgSeq> args;
    mutable std::optional<Type> cached_rettype;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;
};

class NFunctionSelfCall : public NExpr {
public:
    NFunctionSelfCall() = default;
    NFunctionSelfCall(std::unique_ptr<NExpr> p, std::string n, std::unique_ptr<NArgSeq> a);
    std::unique_ptr<NExpr> prefix;
    std::string name;
    std::unique_ptr<NArgSeq> args;
    mutable std::optional<Type> cached_type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;
};

class NNumberLiteral : public NExpr {
public:
    NNumberLiteral() = default;
    NNumberLiteral(std::string v);
    std::string value;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;
};

class NAssignment : public Node {
public:
    NAssignment() = default;
    NAssignment(std::vector<std::unique_ptr<NExpr>> v, std::vector<std::unique_ptr<NExpr>> e);
    std::vector<std::unique_ptr<NExpr>> vars;
    std::vector<std::unique_ptr<NExpr>> exprs;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NEmpty : public Node {
public:
    NEmpty() = default;

    virtual void dump(std::ostream& out) const override;
};

class NLabel : public Node {
public:
    NLabel() = default;
    NLabel(std::string n);
    std::string name;

    virtual void dump(std::ostream& out) const override;
};

class NBreak : public Node {
public:
    NBreak() = default;

    virtual void dump(std::ostream& out) const override;
};

class NGoto : public Node {
public:
    NGoto() = default;
    NGoto(std::string n);
    std::string name;

    virtual void dump(std::ostream& out) const override;
};

class NWhile : public Node {
public:
    NWhile() = default;
    NWhile(std::unique_ptr<NExpr> c, std::unique_ptr<NBlock> b);
    std::unique_ptr<NExpr> condition;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NRepeat : public Node {
public:
    NRepeat() = default;
    NRepeat(std::unique_ptr<NBlock> b, std::unique_ptr<NExpr> u);
    std::unique_ptr<NBlock> block;
    std::unique_ptr<NExpr> until;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NElseIf : public Node {
public:
    NElseIf() = default;
    NElseIf(std::unique_ptr<NExpr> c, std::unique_ptr<NBlock> b);
    std::unique_ptr<NExpr> condition;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NElse : public Node {
public:
    NElse() = default;
    NElse(std::unique_ptr<NBlock> b);
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NIf : public Node {
public:
    NIf() = default;
    NIf(std::unique_ptr<NExpr> c, std::unique_ptr<NBlock> b, std::vector<std::unique_ptr<NElseIf>> ei, std::unique_ptr<NElse> e);
    std::unique_ptr<NExpr> condition;
    std::unique_ptr<NBlock> block;
    std::vector<std::unique_ptr<NElseIf>> elseifs;
    std::unique_ptr<NElse> else_;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NForNumeric : public Node {
public:
    NForNumeric() = default;
    NForNumeric(std::string n, std::unique_ptr<NExpr> b, std::unique_ptr<NExpr> e, std::unique_ptr<NExpr> s, std::unique_ptr<NBlock> k);
    std::string name;
    std::unique_ptr<NExpr> begin;
    std::unique_ptr<NExpr> end;
    std::unique_ptr<NExpr> step;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NForGeneric : public Node {
public:
    NForGeneric() = default;
    NForGeneric(std::vector<NNameDecl> n, std::vector<std::unique_ptr<NExpr>> e, std::unique_ptr<NBlock> b);
    std::vector<NNameDecl> names;
    std::vector<std::unique_ptr<NExpr>> exprs;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NFuncParams : public Node {
public:
    NFuncParams() = default;
    NFuncParams(std::vector<NNameDecl> n, bool v);
    std::vector<NNameDecl> names;
    bool is_variadic = false;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    void add_to_scope(Scope& scope) const;

    std::vector<Type> get_types(const Scope& scope) const;
};

class FunctionBase {
public:
    FunctionBase() = default;
    FunctionBase(std::vector<NNameDecl> g, std::unique_ptr<NFuncParams> p, std::unique_ptr<NType> r, std::unique_ptr<NBlock> b);

    Type check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    Type get_type(Scope& parent_scope, const Type& rettype) const;

    std::vector<NNameDecl> generic_params;
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NType> ret;
    std::unique_ptr<NBlock> block;
};

class NFunction : public Node {
public:
    NFunction() = default;
    NFunction(FunctionBase fb, std::unique_ptr<NExpr> e);
    FunctionBase base;
    std::unique_ptr<NExpr> expr;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;
    virtual void dump(std::ostream& out) const override;
};

class NSelfFunction : public Node {
public:
    NSelfFunction() = default;
    NSelfFunction(
        std::string m,
        std::unique_ptr<NExpr> e,
        std::unique_ptr<NFuncParams> p,
        std::unique_ptr<NType> r,
        std::unique_ptr<NBlock> b);
    std::string name;
    std::unique_ptr<NExpr> expr;
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NType> ret;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NLocalFunction : public Node {
public:
    NLocalFunction() = default;
    NLocalFunction(std::string n, std::unique_ptr<NFuncParams> p, std::unique_ptr<NType> r, std::unique_ptr<NBlock> b);
    std::string name;
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NType> ret;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NReturn : public Node {
public:
    NReturn() = default;
    NReturn(std::vector<std::unique_ptr<NExpr>> e);
    std::vector<std::unique_ptr<NExpr>> exprs;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NLocalVar : public Node {
public:
    NLocalVar() = default;
    NLocalVar(std::vector<NNameDecl> n, std::vector<std::unique_ptr<NExpr>> e);
    std::vector<NNameDecl> names;
    std::vector<std::unique_ptr<NExpr>> exprs;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NGlobalVar : public Node {
public:
    NGlobalVar() = default;
    NGlobalVar(std::vector<NNameDecl> n, std::vector<std::unique_ptr<NExpr>> e);
    std::vector<NNameDecl> names;
    std::vector<std::unique_ptr<NExpr>> exprs;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
};

class NNil : public NExpr {
public:
    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;
};

class NBooleanLiteral : public NExpr {
public:
    NBooleanLiteral() = default;
    NBooleanLiteral(bool v);
    bool value;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;
};

class NStringLiteral : public NExpr {
public:
    NStringLiteral() = default;
    NStringLiteral(std::string v);
    std::string value;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;
};

class NDots : public NExpr {
public:
    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;
};

class NFunctionDef : public NExpr {
public:
    NFunctionDef() = default;
    NFunctionDef(std::unique_ptr<NFuncParams> p, std::unique_ptr<NType> r, std::unique_ptr<NBlock> b);
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NType> ret;
    std::unique_ptr<NBlock> block;
    mutable std::optional<Type> deducedret;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;
};

class NField : public Node {
public:
    virtual void add_to_table(const Scope& scope, std::vector<KeyValPair>& indexes, FieldMap& fielddecls, std::vector<CompileError>& errors)
        const = 0;
};

class NFieldExpr : public NField {
public:
    NFieldExpr() = default;
    NFieldExpr(std::unique_ptr<NExpr> e);
    std::unique_ptr<NExpr> expr;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    virtual void add_to_table(const Scope& scope, std::vector<KeyValPair>& indexes, FieldMap& fielddecls, std::vector<CompileError>& errors)
        const override;
};

class NFieldNamed : public NField {
public:
    NFieldNamed() = default;
    NFieldNamed(std::string k, std::unique_ptr<NExpr> v);
    std::string key;
    std::unique_ptr<NExpr> value;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;
    virtual void dump(std::ostream& out) const override;
    virtual void add_to_table(const Scope& scope, std::vector<KeyValPair>& indexes, FieldMap& fielddecls, std::vector<CompileError>& errors)
        const override;
};

class NFieldKey : public NField {
public:
    NFieldKey() = default;
    NFieldKey(std::unique_ptr<NExpr> k, std::unique_ptr<NExpr> v);
    std::unique_ptr<NExpr> key;
    std::unique_ptr<NExpr> value;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
    virtual void add_to_table(const Scope& scope, std::vector<KeyValPair>& indexes, FieldMap& fielddecls, std::vector<CompileError>& errors)
        const override;
};

class NTableConstructor : public NExpr {
public:
    NTableConstructor() = default;
    NTableConstructor(std::vector<std::unique_ptr<NField>> f);
    std::vector<std::unique_ptr<NField>> fields;
    mutable std::optional<Type> cached_type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;
    virtual Type get_type(const Scope& scope) const override;
};

class NBinop : public NExpr {
public:
    enum class Op {
        OR,
        AND,
        LT,
        GT,
        LEQ,
        GEQ,
        NEQ,
        EQ,
        BOR,
        BXOR,
        BAND,
        SHL,
        SHR,
        CONCAT,
        ADD,
        SUB,
        MUL,
        DIV,
        IDIV,
        MOD,
        POW
    };

    NBinop() = default;
    NBinop(Op o, std::unique_ptr<NExpr> l, std::unique_ptr<NExpr> r);
    Op op;
    std::unique_ptr<NExpr> left;
    std::unique_ptr<NExpr> right;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;
};

class NUnaryop : public NExpr {
public:
    enum class Op {
        NOT,
        LEN,
        NEG,
        BNOT
    };

    NUnaryop() = default;
    NUnaryop(Op o, std::unique_ptr<NExpr> e);
    Op op;
    std::unique_ptr<NExpr> expr;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const;

    virtual void dump(std::ostream& out) const override;

    virtual Type get_type(const Scope& scope) const override;
};

} // namespace typedlua::ast

#endif // TYPEDLUA_NODE_HPP
