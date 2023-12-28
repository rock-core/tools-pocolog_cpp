#include <pocolog_cpp/OwnedValue.hpp>
#include <typelib/value_ops.hh>

using namespace pocolog_cpp;

OwnedValue::OwnedValue(Typelib::Type const& type)
    : buffer(type.getSize())
    , value(buffer.data(), type) {
    Typelib::init(value);
}

OwnedValue::OwnedValue(Typelib::Value const& from)
    : buffer(from.getType().getSize())
    , value(buffer.data(), from.getType()) {
    Typelib::copy(value, from);
}

OwnedValue::OwnedValue(OwnedValue const& from)
    : OwnedValue(from.value) {}

OwnedValue::OwnedValue(OwnedValue&& from)
    : buffer(std::move(from.buffer))
    , value(from.value) {
    from.value = Typelib::Value(nullptr, from.getType());
}

void OwnedValue::load(std::vector<uint8_t> const& marshalled_buffer) {
    Typelib::load(value, marshalled_buffer);
}

Typelib::Type const& OwnedValue::getType() const {
    return value.getType();
}

Typelib::Value OwnedValue::operator*() const {
    return value;
}
