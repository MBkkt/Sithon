#include "statement.h"
#include "object.h"

#include <iostream>
#include <sstream>


using namespace std;

namespace Ast {

using Runtime::Closure;

ObjectHolder Assignment::Execute(Closure &closure) {
    return closure[var_name] = right_value->Execute(closure);
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
    : var_name(std::move(var)), right_value(std::move(rv)) {
}

VariableValue::VariableValue(std::string var_name)
    : dotted_ids(1, std::move(var_name)) {
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids)
    : dotted_ids(std::move(dotted_ids)) {
    if (this->dotted_ids.empty()) {
        throw std::runtime_error("You can't create VariableValue with empty dotted_ids");
    }
}

ObjectHolder VariableValue::Execute(Closure &closure) {
    auto *cur_closure = &closure;

    for (size_t i = 0; i + 1 < dotted_ids.size(); ++i) {
        if (auto it = cur_closure->find(dotted_ids[i]); it == cur_closure->end()) {
            throw std::runtime_error("Name " + dotted_ids[i] + " not found in the scope");
        } else if (auto p = it->second.TryAs<Runtime::ClassInstance>(); p) {
            cur_closure = &p->Fields();
        } else {
            throw std::runtime_error(dotted_ids[i] + " is not an object, can't access its fields");
        }
    }

    if (auto it = cur_closure->find(dotted_ids.back()); it != closure.end()) {
        return it->second;
    } else {
        throw std::runtime_error("Variable " + dotted_ids.back() + " not found in closure");
    }
}

unique_ptr<Print> Print::Variable(std::string var) {
    return make_unique<Print>(make_unique<VariableValue>(std::move(var)));
}

Print::Print(unique_ptr<Statement> argument) {
    args.push_back(std::move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args)
    : args(std::move(args)) {
}

ObjectHolder Print::Execute(Closure &closure) {
    bool first = true;
    for (const auto &statement : args) {
        if (!first) {
            (*output) << ' ';
        }
        first = false;

        if (ObjectHolder result = statement->Execute(closure)) {
            result->Print(*output);
        } else {
            (*output) << "None";
        }
    }
    (*output) << '\n';
    return ObjectHolder::None();
}

ostream *Print::output = &cout;

void Print::SetOutputStream(ostream &output_stream) {
    output = &output_stream;
}

MethodCall::MethodCall(
    std::unique_ptr<Statement> object, std::string method, std::vector<std::unique_ptr<Statement>> args
)
    : object(std::move(object)), method(std::move(method)), args(std::move(args)) {
}

ObjectHolder MethodCall::Execute(Closure &closure) {
    vector<ObjectHolder> actual_args;
    for (auto &stmt : args) {
        actual_args.push_back(stmt->Execute(closure));
    }

    ObjectHolder callee = object->Execute(closure);
    if (auto *instance = callee.TryAs<Runtime::ClassInstance>(); instance) {
        return instance->Call(method, actual_args);
    } else {
        throw std::runtime_error("Trying to call method " + method + " on object whicj is not a class instance");
    }
}

ObjectHolder Stringify::Execute(Closure &closure) {
    ObjectHolder arg_value = argument->Execute(closure);

    std::ostringstream os;
    arg_value->Print(os);

    return ObjectHolder::Own(Runtime::String(os.str()));
}

template<typename T>
bool TryAddValues(const ObjectHolder &left, const ObjectHolder &right, ObjectHolder &result) {
    auto l = left.TryAs<T>();
    auto r = right.TryAs<T>();
    if (l && r) {
        result = ObjectHolder::Own(T(l->GetValue() + r->GetValue()));
        return true;
    }
    return false;
}

bool TryAddInstances(ObjectHolder &left, ObjectHolder &right, ObjectHolder &result) {
    if (auto l = left.TryAs<Runtime::ClassInstance>(); !l) {
        return false;
    } else if (l->HasMethod("__add__", 1)) {
        result = l->Call("__add__", {right});
        return true;
    } else {
        return false;
    }
}

ObjectHolder Add::Execute(Closure &closure) {
    auto left = lhs->Execute(closure);
    auto right = rhs->Execute(closure);

    ObjectHolder result;

    bool success = TryAddValues<Runtime::Number>(left, right, result);
    success = success || TryAddValues<Runtime::String>(left, right, result);
    success = success || TryAddInstances(left, right, result);

    if (success) {
        return result;
    } else {
        throw std::runtime_error("Addition isn't supported for these operands");
    }
}

ObjectHolder Sub::Execute(Closure &closure) {
    auto left = lhs->Execute(closure);
    auto right = rhs->Execute(closure);

    auto left_number = left.TryAs<Runtime::Number>();
    auto right_number = right.TryAs<Runtime::Number>();

    if (left_number && right_number) {
        return ObjectHolder::Own(
            Runtime::Number(left_number->GetValue() - right_number->GetValue())
        );
    } else {
        throw std::runtime_error("Subtraction is supported only for integers");
    }
}

ObjectHolder Mult::Execute(Runtime::Closure &closure) {
    auto left = lhs->Execute(closure);
    auto right = rhs->Execute(closure);

    auto left_number = left.TryAs<Runtime::Number>();
    auto right_number = right.TryAs<Runtime::Number>();

    if (left_number && right_number) {
        return ObjectHolder::Own(
            Runtime::Number(left_number->GetValue() * right_number->GetValue())
        );
    } else {
        throw std::runtime_error("Multiplication is supported only for integers");
    }
}

ObjectHolder Div::Execute(Runtime::Closure &closure) {
    auto left = lhs->Execute(closure);
    auto right = rhs->Execute(closure);

    auto left_number = left.TryAs<Runtime::Number>();
    auto right_number = right.TryAs<Runtime::Number>();

    if (!left_number || !right_number) {
        throw std::runtime_error("Multiplication is supported only for integers");
    } else if (right_number->GetValue() == 0) {
        throw std::runtime_error("Division by zero");
    } else {
        return ObjectHolder::Own(
            Runtime::Number(left_number->GetValue() / right_number->GetValue())
        );
    }
}

ObjectHolder Compound::Execute(Closure &closure) {
    for (auto &stmt : statements) {
        stmt->Execute(closure);
    }
    return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure &closure) {
    throw statement->Execute(closure);
}

ClassDefinition::ClassDefinition(ObjectHolder class_)
    : cls(std::move(class_)), class_name(dynamic_cast<const Runtime::Class &>(*cls).GetName()) {
}

ObjectHolder ClassDefinition::Execute(Runtime::Closure &closure) {
    closure[class_name] = cls;
    return ObjectHolder::None();
}

FieldAssignment::FieldAssignment(
    VariableValue object, std::string field_name, std::unique_ptr<Statement> rv
)
    : object(std::move(object)), field_name(std::move(field_name)), right_value(std::move(rv)) {
}

ObjectHolder FieldAssignment::Execute(Runtime::Closure &closure) {
    auto instance = object.Execute(closure);
    if (auto p = instance.TryAs<Runtime::ClassInstance>(); p) {
        return p->Fields()[field_name] = right_value->Execute(closure);
    } else {
        throw std::runtime_error("Cannot assign to the field " + field_name + " of not an object");
    }
}

IfElse::IfElse(
    std::unique_ptr<Statement> condition,
    std::unique_ptr<Statement> if_body,
    std::unique_ptr<Statement> else_body
)
    : condition(std::move(condition)), if_body(std::move(if_body)), else_body(std::move(else_body)) {
}

ObjectHolder IfElse::Execute(Runtime::Closure &closure) {
    auto value = condition->Execute(closure);
    if (IsTrue(value)) {
        if_body->Execute(closure);
    } else if (else_body) {
        else_body->Execute(closure);
    }
    return ObjectHolder::None();
}

ObjectHolder Or::Execute(Runtime::Closure &closure) {
    if (IsTrue(lhs->Execute(closure)) || IsTrue(rhs->Execute(closure))) {
        return ObjectHolder::Own(Runtime::Bool(true));
    } else {
        return ObjectHolder::Own(Runtime::Bool(false));
    }
}

ObjectHolder And::Execute(Runtime::Closure &closure) {
    if (IsTrue(lhs->Execute(closure)) && IsTrue(rhs->Execute(closure))) {
        return ObjectHolder::Own(Runtime::Bool(true));
    } else {
        return ObjectHolder::Own(Runtime::Bool(false));
    }
}

ObjectHolder Not::Execute(Runtime::Closure &closure) {
    return ObjectHolder::Own(Runtime::Bool(!IsTrue(argument->Execute(closure))));
}

Comparison::Comparison(
    Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs
)
    : comparator(std::move(cmp)), left(std::move(lhs)), right(std::move(rhs)) {
}

ObjectHolder Comparison::Execute(Runtime::Closure &closure) {
    return ObjectHolder::Own(
        Runtime::Bool(comparator(left->Execute(closure), right->Execute(closure)))
    );
}

NewInstance::NewInstance(
    const Runtime::Class &class_, std::vector<std::unique_ptr<Statement>> args
)
    : class_(class_), args(std::move(args)) {
}

NewInstance::NewInstance(const Runtime::Class &class_) : NewInstance(class_, {}) {
}

ObjectHolder NewInstance::Execute(Runtime::Closure &closure) {
    Runtime::ClassInstance result(class_);
    if (auto *m = class_.GetMethod("__init__"); m) {
        vector<ObjectHolder> actual_args;
        for (auto &stmt : args) {
            actual_args.push_back(stmt->Execute(closure));
        }

        result.Call("__init__", actual_args);
    }
    return ObjectHolder::Own(std::move(result));
}


} /* namespace Ast */
