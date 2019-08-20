#include "object.h"
#include "statement.h"

#include <sstream>
#include <string_view>


using namespace std;

namespace Runtime {

void ClassInstance::Print(std::ostream &os) {
    if (HasMethod("__str__", 0)) {
        Call("__str__", {})->Print(os);
    } else {
        os << this;
    }
}

bool ClassInstance::HasMethod(const std::string &method, size_t argument_count) const {
    auto *m = class_.GetMethod(method);
    return m && m->formal_params.size() == argument_count;
}

bool StartsWith(string_view what, string_view with) {
    return what.substr(0, with.size()) == with;
}

ClassInstance::ClassInstance(const Class &cls) : class_(cls) {
}

ObjectHolder ClassInstance::Call(const std::string &method, const std::vector<ObjectHolder> &actual_args) {
    if (auto *m = class_.GetMethod(method); !m) {
        throw std::runtime_error("Class " + class_.GetName() + " doesn't have method " + method);
    } else if (m->formal_params.size() != actual_args.size()) {
        std::ostringstream msg;
        msg << "Method " << class_.GetName() << "::" << method << " expects "
            << m->formal_params.size() << " arguments, but " << actual_args.size() << " given";
        throw std::runtime_error(msg.str());
    } else {
        try {
            Closure closure = {{"self", ObjectHolder::Share(*this)}};
            for (size_t i = 0; i < actual_args.size(); ++i) {
                closure[m->formal_params[i]] = actual_args[i];
            }
            return m->body->Execute(closure);
        } catch (ObjectHolder &returned_value) {
            return returned_value;
        }
        return ObjectHolder::None();
    }
}

Class::Class(std::string name, std::vector<Method> methods, const Class *parent)
    : class_name(std::move(name)), parent(parent) {
    for (auto &m : methods) {
        if (vmt.find(m.name) != vmt.end()) {
            throw runtime_error("Class " + class_name + " has duplicate methods with name " + m.name);
        } else {
            vmt[m.name] = std::move(m);
        }
    }
}

const Method *Class::GetMethod(const std::string &name) const {
    if (auto it = vmt.find(name); it != vmt.end()) {
        return &it->second;
    } else if (parent) {
        return parent->GetMethod(name);
    } else {
        return nullptr;
    }
}

void Class::Print(ostream &os) {
    os << "Class " << class_name;
}

void Bool::Print(std::ostream &os) {
    os << (GetValue() ? "True" : "False");
}

} /* namespace Runtime */
