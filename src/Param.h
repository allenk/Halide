#ifndef HALIDE_PARAM_H
#define HALIDE_PARAM_H

/** \file
 *
 * Classes for declaring scalar parameters to halide pipelines
 */

namespace Halide {

/** A struct used to detect if a type is a pointer. If it's not a
 * pointer, then not_a_pointer<T>::type is T.  If it is a pointer,
 * then not_a_pointer<T>::type is some internal hidden type that no
 * overload should trigger on. TODO: with C++11 this can be written
 * more cleanly. */
namespace Internal {
template<typename T> struct not_a_pointer {typedef T type;};
template<typename T> struct not_a_pointer<T *> { struct type {}; };
}

/** A scalar parameter to a halide pipeline. If you're jitting, this
 * should be bound to an actual value of type T using the set method
 * before you realize the function uses this. If you're statically
 * compiling, this param should appear in the argument list. */
template<typename T>
class Param {
    /** A reference-counted handle on the internal parameter object */
    Internal::Parameter param;

    void check_name() const {
        user_assert(param.name() != "__user_context") << "Param<void*>(\"__user_context\") "
            << "is no longer used to control whether Halide functions take explicit "
            << "user_context arguments. Use set_custom_user_context() when jitting, "
            << "or add Target::UserContext to the Target feature set when compiling ahead of time.";
    }

public:
    /** Construct a scalar parameter of type T with a unique
     * auto-generated name */
    Param() :
        param(type_of<T>(), false, 0, Internal::make_entity_name(this, "Halide::Param<?", 'p')) {}

    /** Construct a scalar parameter of type T with the given name. */
    // @{
    explicit Param(const std::string &n) :
        param(type_of<T>(), false, 0, n, /*is_explicit_name*/ true) {
        check_name();
    }
    explicit Param(const char *n) :
        param(type_of<T>(), false, 0, n, /*is_explicit_name*/ true) {
        check_name();
    }
    // @}

    /** Construct a scalar parameter of type T an initial value of
     * 'val'. Only triggers for scalar types. */
    explicit Param(typename Internal::not_a_pointer<T>::type val) :
        param(type_of<T>(), false, 0, Internal::make_entity_name(this, "Halide::Param<?", 'p')) {
        set(val);
    }

    /** Construct a scalar parameter of type T with the given name
     * and an initial value of 'val'. */
    Param(const std::string &n, T val) :
        param(type_of<T>(), false, 0, n, /*is_explicit_name*/ true) {
        check_name();
        set(val);
    }

    /** Construct a scalar parameter of type T with an initial value of 'val'
    * and a given min and max. */
    Param(T val, Expr min, Expr max) :
        param(type_of<T>(), false, 0, Internal::make_entity_name(this, "Halide::Param<?", 'p')) {
        set_range(min, max);
        set(val);
    }

    /** Construct a scalar parameter of type T with the given name
     * and an initial value of 'val' and a given min and max. */
    Param(const std::string &n, T val, Expr min, Expr max) :
        param(type_of<T>(), false, 0, n, /*is_explicit_name*/ true) {
        check_name();
        set_range(min, max);
        set(val);
    }

    /** Get the name of this parameter */
    const std::string &name() const {
        return param.name();
    }

    /** Return true iff the name was explicitly specified in the ctor (vs autogenerated). */
    bool is_explicit_name() const {
        return param.is_explicit_name();
    }

    /** Get the current value of this parameter. Only meaningful when jitting. */
    NO_INLINE T get() const {
        return param.get_scalar<T>();
    }

    /** Set the current value of this parameter. Only meaningful when jitting */
    NO_INLINE void set(T val) {
        param.set_scalar<T>(val);
    }

    /** Get a pointer to the location that stores the current value of
     * this parameter. Only meaningful for jitting. */
    NO_INLINE T *get_address() const {
        return (T *)(param.get_scalar_address());
    }

    /** Get the halide type of T */
    Type type() const {
        return type_of<T>();
    }

    /** Get or set the possible range of this parameter. Use undefined
     * Exprs to mean unbounded. */
    // @{
    void set_range(Expr min, Expr max) {
        set_min_value(min);
        set_max_value(max);
    }

    void set_min_value(Expr min) {
        if (min.type() != type_of<T>()) {
            min = Internal::Cast::make(type_of<T>(), min);
        }
        param.set_min_value(min);
    }

    void set_max_value(Expr max) {
        if (max.type() != type_of<T>()) {
            max = Internal::Cast::make(type_of<T>(), max);
        }
        param.set_max_value(max);
    }

    Expr get_min_value() const {
        return param.get_min_value();
    }

    Expr get_max_value() const {
        return param.get_max_value();
    }
    // @}

    void set_default_value(const T &value) {
        param.set_default(value);
    }

    /** You can use this parameter as an expression in a halide
     * function definition */
    operator Expr() const {
        return Internal::Variable::make(type_of<T>(), name(), param);
    }

    /** Using a param as the argument to an external stage treats it
     * as an Expr */
    operator ExternFuncArgument() const {
        return Expr(*this);
    }

    /** Construct the appropriate argument matching this parameter,
     * for the purpose of generating the right type signature when
     * statically compiling halide pipelines. */
    operator Argument() const {
        return Argument(name(), Argument::InputScalar, type(), 0,
            param.get_scalar_expr(), param.get_min_value(), param.get_max_value());
    }
};

/** Returns an Expr corresponding to the user context passed to
 * the function (if any). It is rare that this function is necessary
 * (e.g. to pass the user context to an extern function written in C). */
inline Expr user_context_value() {
    return Internal::Variable::make(Handle(), "__user_context",
        Internal::Parameter(Handle(), false, 0, "__user_context", true));
}

}

#endif
