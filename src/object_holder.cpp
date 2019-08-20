#include "object_holder.h"
#include "object.h"


namespace Runtime {

ObjectHolder ObjectHolder::Share(Object &object) {
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto *) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object &ObjectHolder::operator*() {
    return *Get();
}

const Object &ObjectHolder::operator*() const {
    return *Get();
}

Object *ObjectHolder::operator->() {
    return Get();
}

const Object *ObjectHolder::operator->() const {
    return Get();
}

Object *ObjectHolder::Get() {
    return data.get();
}

const Object *ObjectHolder::Get() const {
    return data.get();
}

ObjectHolder::operator bool() const {
    return Get();
}

bool IsTrue(ObjectHolder object) {
    if (!object) {
        return false;
    }
    if (auto p = object.TryAs<Runtime::Number>(); p && p->GetValue() != 0) {
        return true;
    }
    if (auto p = object.TryAs<Runtime::String>(); p && !p->GetValue().empty()) {
        return true;
    }
    if (auto p = object.TryAs<Runtime::Bool>(); p && p->GetValue()) {
        return true;
    }
    return false;
}

}
