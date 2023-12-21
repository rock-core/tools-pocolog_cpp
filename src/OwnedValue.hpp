#ifndef POCOLOG_CPP_OWNEDVALUE_HPP
#define POCOLOG_CPP_OWNEDVALUE_HPP

#include <vector>
#include <typelib/value.hh>

namespace pocolog_cpp {

/** Wrapper over Typelib::Value where the buffer is owned by the value */
class OwnedValue {
    std::vector<uint8_t> buffer;
    Typelib::Value value;

public:
    OwnedValue(Typelib::Type const& type);
    OwnedValue(Typelib::Value const& from);
    OwnedValue(OwnedValue const& from);
    OwnedValue(OwnedValue&& from);

    Typelib::Type const& getType() const;

    void load(std::vector<uint8_t> const& marshalled_buffer);

    template<typename T>
    T const& get(bool size_check = true) const {
        if (size_check && sizeof(T) != buffer.size()) {
            throw std::logic_error("in-process and typelib sizes differ");
        }

        return reinterpret_cast<T const&>(*buffer.data());
    }
};

}

#endif