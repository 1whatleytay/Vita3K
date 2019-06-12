#ifndef VITA3K_OPTIONAL_H
#define VITA3K_OPTIONAL_H

#include <exception>

template <typename T>
class Optional {
    bool hasValue = false;
    T value;
public:
    operator bool() const {
        return hasValue;
    }

    T &getValue() {
        if (hasValue)
            return value;
        else
            throw std::exception();
    }

    Optional(T value) : hasValue(true), value(value) { }
    Optional() = default;
};

#endif //VITA3K_OPTIONAL_H
