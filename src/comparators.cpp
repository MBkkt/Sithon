#include "comparators.h"
#include "object.h"
#include "object_holder.h"

#include <functional>
#include <optional>
#include <sstream>


using namespace std;

namespace Runtime {

template<typename T, typename Cmp>
optional<bool> TryCompare(const ObjectHolder &lhs, const ObjectHolder &rhs, Cmp cmp) {
    auto l = lhs.TryAs<T>();
    auto r = rhs.TryAs<T>();
    if (l && r) {
        return cmp(l->GetValue(), r->GetValue());
    } else {
        return nullopt;
    }
}

bool Equal(ObjectHolder lhs, ObjectHolder rhs) {
    auto result = TryCompare<Runtime::Number>(lhs, rhs, std::equal_to<int>());
    if (!result) {
        result = TryCompare<Runtime::String>(lhs, rhs, std::equal_to<string>());
    }
    if (!result) {
        result = TryCompare<Runtime::Bool>(lhs, rhs, std::equal_to<bool>());
    }
    if (result) {
        return *result;
    }
    if (auto p = lhs.TryAs<Runtime::ClassInstance>(); p && p->HasMethod("__eq__", 1)) {
        return IsTrue(p->Call("__eq__", {rhs}));
    }
    if (!lhs && !rhs) {
        return true;
    }

    throw std::runtime_error("Cannot compare objects for equality");
}

bool Less(ObjectHolder lhs, ObjectHolder rhs) {
    auto result = TryCompare<Runtime::Number>(lhs, rhs, std::less<int>());
    if (!result) {
        result = TryCompare<Runtime::String>(lhs, rhs, std::less<string>());
    }
    if (!result) {
        result = TryCompare<Runtime::Bool>(lhs, rhs, std::less<bool>());
    }
    if (result) {
        return *result;
    }
    if (auto p = lhs.TryAs<Runtime::ClassInstance>(); p && p->HasMethod("__lt__", 1)) {
        return IsTrue(p->Call("__lt__", {rhs}));
    }

    throw std::runtime_error("Cannot compare objects for less");
}


} /* namespace Runtime */
