#ifndef EXPECTED_HPP
#define EXPECTED_HPP

#include <type_traits>
#include <memory>

template <typename ErrorType>
class Unexpected {
public:
    using ErrorTypeValue = std::remove_cvref_t<ErrorType>;

private:
    const ErrorTypeValue error;

public:
    Unexpected(const ErrorTypeValue& val) : error{val} {}
    Unexpected(ErrorTypeValue&& val) : error{std::move(val)} {}

    Unexpected(const Unexpected& err) : error{err.error} {}
    Unexpected(Unexpected&& err) : error{std::move(err.error)} {}

    Unexpected& operator=(const Unexpected&) = delete;
    Unexpected& operator=(Unexpected&&) = delete;

    inline const ErrorTypeValue What() const { return error; }
};

template <typename Type, typename ErrorType>
class Expected {
public:
    using TypeValue = std::remove_cvref_t<Type>;
    using ErrorTypeValue = Unexpected<ErrorType>::ErrorTypeValue;

private:
    std::unique_ptr<TypeValue> value = nullptr;
    std::unique_ptr<Unexpected<ErrorType>> error = nullptr;

public:
    Expected(const TypeValue& val) : value{std::make_unique<TypeValue>(val)} {}
    Expected(TypeValue&& val) : value{std::make_unique<TypeValue>(std::move(val))} {}
    
    Expected(const Unexpected<ErrorType>& err) : error{std::make_unique<Unexpected<ErrorType>>(err)} {}
    Expected(Unexpected<ErrorType>&& err) : error{std::make_unique<Unexpected<ErrorType>>(std::move(err))} {}

    Expected(const Expected& exp) {
        if (exp.value)
            value = std::make_unique<TypeValue>(*exp.value);
        
        if (exp.error)
            error = std::make_unique<Unexpected<ErrorType>>(*exp.error);
    }

    Expected(Expected&& exp) : value{std::move(exp.value)}, error{std::move(exp.error)} {}

    Expected& operator=(const Expected& exp) {
        if (exp.value)
            value = std::make_unique<TypeValue>(*exp.value);
        
        if (exp.error)
            error = std::make_unique<Unexpected<ErrorType>>(*exp.error);
        
        return *this;
    }

    Expected& operator=(Expected&& exp) {
        value = std::move(exp.value);
        error = std::move(exp.error);

        return *this;
    }

    bool HasValue() const {
        return value != nullptr;
    }

    bool HasErrorType() const {
        return error != nullptr;
    }

    TypeValue Value() const {
        return *value;
    }

    TypeValue ValueOr(const TypeValue& default_value) const {
        if (HasValue())
            return *value;
        
        return default_value;
    }

    ErrorTypeValue Error() const {
        if (HasErrorType())
            return error->What();
        
        return ErrorTypeValue();
    }

    operator bool() const {
        return HasValue();
    }
};

template <typename Type, typename ErrorType>
class Expected<Type&, ErrorType> {
public:
    using TypeValue = std::remove_cvref_t<Type>;
    using ErrorTypeValue = Unexpected<ErrorType>::ErrorTypeValue;

private:
    TypeValue* value = nullptr;
    std::unique_ptr<Unexpected<ErrorType>> error = nullptr;

public:
    Expected(TypeValue& val) : value{&val} {}
    Expected(TypeValue&& val) = delete;
    
    Expected(const Unexpected<ErrorType>& err) : error{std::make_unique<Unexpected<ErrorType>>(err)} {}
    Expected(Unexpected<ErrorType>&& err) : error{std::make_unique<Unexpected<ErrorType>>(std::move(err))} {}

    Expected(const Expected& exp) {
        if (exp.value)
            value = exp.value;
        
        if (exp.error)
            error = std::make_unique<Unexpected<ErrorType>>(*exp.error);
    }

    Expected(Expected&& exp) : value{std::move(exp.value)}, error{std::move(exp.error)} {}

    Expected& operator=(const Expected& exp) {
        if (exp.value)
            value = exp.value;
        
        if (exp.error)
            error = std::make_unique<Unexpected<ErrorType>>(*exp.error);
        
        return *this;
    }

    Expected& operator=(Expected&& exp) {
        value = std::move(exp.value);
        error = std::move(exp.error);

        return *this;
    }

    bool HasValue() const {
        return value != nullptr;
    }

    bool HasErrorType() const {
        return error != nullptr;
    }

    TypeValue Value() const {
        return *value;
    }

    TypeValue ValueOr(const TypeValue& default_value) const {
        if (HasValue())
            return *value;
        
        return default_value;
    }

    ErrorTypeValue Error() const {
        if (HasErrorType())
            return error->What();
        
        return ErrorTypeValue();
    }

    operator bool() const {
        return HasValue();
    }
};

#endif