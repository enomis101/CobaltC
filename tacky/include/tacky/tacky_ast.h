#pragma once
#include <string>
#include <variant>
#include <vector>

namespace tacky {

// Forward declarations for all node types
struct Identifier;
struct Constant;
struct TemporaryVariable;
struct ReturnInstruction;
struct UnaryInstruction;
struct BinaryInstruction;
struct CopyInstruction;
struct JumpInstruction;
struct JumpIfZeroInstruction;
struct JumpIfNotZeroInstruction;
struct LabelInstruction;
struct FunctionCallInstruction;
struct FunctionDefinition;
struct StaticVariable;
struct Program;

// Enums remain the same
enum class UnaryOperator {
    COMPLEMENT,
    NEGATE,
    NOT
};

enum class BinaryOperator {
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    REMAINDER,
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    LESS_OR_EQUAL,
    GREATER_THAN,
    GREATER_OR_EQUAL
};

// Value variant - represents all possible value types
using Value = std::variant<Constant, TemporaryVariable>;

// Instruction variant - represents all possible instruction types
using Instruction = std::variant<
    ReturnInstruction,
    UnaryInstruction,
    BinaryInstruction,
    CopyInstruction,
    JumpInstruction,
    JumpIfZeroInstruction,
    JumpIfNotZeroInstruction,
    LabelInstruction,
    FunctionCallInstruction>;

// TopLevel variant - represents top-level definitions
using TopLevel = std::variant<FunctionDefinition, StaticVariable>;

// Node definitions - now simple structs with no inheritance
struct Identifier {
    std::string name;

    // Optimized constructor with perfect forwarding
    template<typename T>
    explicit Identifier(T&& n)
        : name(std::forward<T>(n))
    {
    }
};

struct Constant {
    int value;

    explicit Constant(int v)
        : value(v)
    {
    }
};

struct TemporaryVariable {
    Identifier identifier;

    // Constructor with perfect forwarding
    template<typename T>
    explicit TemporaryVariable(T&& id)
        : identifier(std::forward<T>(id))
    {
    }
};

struct ReturnInstruction {
    Value value;

    // Constructor with perfect forwarding for Value
    template<typename T>
    explicit ReturnInstruction(T&& v)
        : value(std::forward<T>(v))
    {
    }
};

struct UnaryInstruction {
    UnaryOperator unary_operator;
    Value source;
    Value destination;

    // Constructor with perfect forwarding
    template<typename S, typename D>
    UnaryInstruction(UnaryOperator op, S&& src, D&& dst)
        : unary_operator(op)
        , source(std::forward<S>(src))
        , destination(std::forward<D>(dst))
    {
    }
};

struct BinaryInstruction {
    BinaryOperator binary_operator;
    Value source1;
    Value source2;
    Value destination;

    // Constructor with perfect forwarding
    template<typename S1, typename S2, typename D>
    BinaryInstruction(BinaryOperator op, S1&& src1, S2&& src2, D&& dst)
        : binary_operator(op)
        , source1(std::forward<S1>(src1))
        , source2(std::forward<S2>(src2))
        , destination(std::forward<D>(dst))
    {
    }
};

struct CopyInstruction {
    Value source;
    Value destination;

    // Constructor with perfect forwarding
    template<typename S, typename D>
    CopyInstruction(S&& src, D&& dst)
        : source(std::forward<S>(src))
        , destination(std::forward<D>(dst))
    {
    }
};

struct JumpInstruction {
    Identifier identifier;

    template<typename T>
    explicit JumpInstruction(T&& id)
        : identifier(std::forward<T>(id))
    {
    }
};

struct JumpIfZeroInstruction {
    Value condition;
    Identifier identifier;

    template<typename C, typename I>
    JumpIfZeroInstruction(C&& cond, I&& id)
        : condition(std::forward<C>(cond))
        , identifier(std::forward<I>(id))
    {
    }
};

struct JumpIfNotZeroInstruction {
    Value condition;
    Identifier identifier;

    template<typename C, typename I>
    JumpIfNotZeroInstruction(C&& cond, I&& id)
        : condition(std::forward<C>(cond))
        , identifier(std::forward<I>(id))
    {
    }
};

struct LabelInstruction {
    Identifier identifier;

    template<typename T>
    explicit LabelInstruction(T&& id)
        : identifier(std::forward<T>(id))
    {
    }
};

struct FunctionCallInstruction {
    Identifier name;
    std::vector<Value> arguments;
    Value destination;

    template<typename N, typename A, typename D>
    FunctionCallInstruction(N&& n, A&& args, D&& dst)
        : name(std::forward<N>(n))
        , arguments(std::forward<A>(args))
        , destination(std::forward<D>(dst))
    {
    }
};

struct FunctionDefinition {
    Identifier name;
    bool global;
    std::vector<Identifier> parameters;
    std::vector<Instruction> body;

    template<typename N, typename P, typename B>
    FunctionDefinition(N&& n, bool glbl, P&& params, B&& b)
        : name(std::forward<N>(n))
        , global(glbl)
        , parameters(std::forward<P>(params))
        , body(std::forward<B>(b))
    {
    }
};

struct StaticVariable {
    Identifier name;
    bool global;
    int init;

    template<typename N>
    StaticVariable(N&& n, bool glbl, int i)
        : name(std::forward<N>(n))
        , global(glbl)
        , init(i)
    {
    }
};

struct Program {
    std::vector<TopLevel> definitions;

    template<typename D>
    explicit Program(D&& defs)
        : definitions(std::forward<D>(defs))
    {
    }
};

// Helper type trait to check if a type is a tacky AST node
template<typename T>
struct is_tacky_node : std::false_type { };

// Specialize for all our node types
template<>
struct is_tacky_node<Identifier> : std::true_type { };
template<>
struct is_tacky_node<Constant> : std::true_type { };
template<>
struct is_tacky_node<TemporaryVariable> : std::true_type { };
template<>
struct is_tacky_node<ReturnInstruction> : std::true_type { };
template<>
struct is_tacky_node<UnaryInstruction> : std::true_type { };
template<>
struct is_tacky_node<BinaryInstruction> : std::true_type { };
template<>
struct is_tacky_node<CopyInstruction> : std::true_type { };
template<>
struct is_tacky_node<JumpInstruction> : std::true_type { };
template<>
struct is_tacky_node<JumpIfZeroInstruction> : std::true_type { };
template<>
struct is_tacky_node<JumpIfNotZeroInstruction> : std::true_type { };
template<>
struct is_tacky_node<LabelInstruction> : std::true_type { };
template<>
struct is_tacky_node<FunctionCallInstruction> : std::true_type { };
template<>
struct is_tacky_node<FunctionDefinition> : std::true_type { };
template<>
struct is_tacky_node<StaticVariable> : std::true_type { };
template<>
struct is_tacky_node<Program> : std::true_type { };

// Visitor pattern using std::visit
// Example visitor base class (users can derive from this)
template<typename R = void>
class TackyVisitor {
public:
    virtual ~TackyVisitor() = default;

    virtual R visit(Identifier& node) = 0;
    virtual R visit(Constant& node) = 0;
    virtual R visit(TemporaryVariable& node) = 0;
    virtual R visit(ReturnInstruction& node) = 0;
    virtual R visit(UnaryInstruction& node) = 0;
    virtual R visit(BinaryInstruction& node) = 0;
    virtual R visit(CopyInstruction& node) = 0;
    virtual R visit(JumpInstruction& node) = 0;
    virtual R visit(JumpIfZeroInstruction& node) = 0;
    virtual R visit(JumpIfNotZeroInstruction& node) = 0;
    virtual R visit(LabelInstruction& node) = 0;
    virtual R visit(FunctionCallInstruction& node) = 0;
    virtual R visit(FunctionDefinition& node) = 0;
    virtual R visit(StaticVariable& node) = 0;
    virtual R visit(Program& node) = 0;

    // Helper to visit a Value variant
    R visit_value(Value& value)
    {
        return std::visit([this](auto& v) -> R { return this->visit(v); }, value);
    }

    // Helper to visit an Instruction variant
    R visit_instruction(Instruction& instruction)
    {
        return std::visit([this](auto& i) -> R { return this->visit(i); }, instruction);
    }

    // Helper to visit a TopLevel variant
    R visit_top_level(TopLevel& top_level)
    {
        return std::visit([this](auto& t) -> R { return this->visit(t); }, top_level);
    }
};

/*
// Alternative: Functional visitor using overloaded lambdas
template<typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

template<typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
*/

} // namespace tacky
