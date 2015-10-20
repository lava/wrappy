#pragma once

#include <vector>
#include <string>
#include <stdexcept>

struct _object;
typedef _object PyObject;

/**
 * Note that this library is *NOT* thread-safe.
 *
 * This is basically unfixable since the reference count of a python object
 * is stored in a non-atomic ssize_t and modified without any locking,
 * making any increases and decreases inherently racy.
 *
 * If you want to use this in a multi-threaded environment, please make sure
 * to protect all accesses to objects that might refer to the same python 
 * object (directly or indirectly) by mutexes.
 */

namespace wrappy {

class WrappyError : public std::runtime_error {
public:
    WrappyError(const std::string& str)
    : std::runtime_error(str)
    { }
};

// A Raii-wrapper around PyObject* that transparently handles
// the necessary reference-counting
class PythonObject {
public:
    // Accessors
    // Python doesn't care about const-ness, so having a separate acccessor
    // for a const PyObject* would be useless. We have to trust the caller
    // not to do non-const things with it.
    long long num() const;
    double floating() const;
    const char* str() const;
    PyObject* get() const;
    PythonObject attr(const std::string& x) const; // returns self.x

    // Constructors
    struct owning {};
    struct borrowed {};
    PythonObject();
    PythonObject(owning, PyObject*);
    PythonObject(borrowed, PyObject*);

    operator bool() const;

    // Rule-of-five plumbing
    ~PythonObject();
    PythonObject(const PythonObject&);
    PythonObject& operator=(const PythonObject&);
    PythonObject(PythonObject&&);
    PythonObject& operator=(PythonObject&&);

private:
    PyObject* obj_;
};

void addModuleSearchPath(const std::string& path);

// There is one quirk of call() for the case of member methods:
//
//     call("module.A.foo") 
//
// calls the unbound method "foo", so it is necessary to provide an instance
// of A as the first argument, while
//
//     auto a = call("module.A"); call(a, "foo"); 
//
// calls the method "foo" that is already bound to a, so providing an explicit
// self argument in that case is an error.

template<typename... Args>
PythonObject call(const std::string& f, Args... args);

template<typename... Args>
PythonObject call(PythonObject from, const std::string& f, Args... args);


// The template-magic in call() constructs a series of appropriate calls
// to these functions, but of course they can also be used directly:

PythonObject construct(long long);
PythonObject construct(int);
PythonObject construct(double);
PythonObject construct(const std::string&);
PythonObject construct(PythonObject); // identity
PythonObject construct(const std::vector<PythonObject>&); // python list

PythonObject callWithArgs(
    const std::string& function,
    const std::vector<PythonObject>& args
        = std::vector<PythonObject>(),
    const std::vector<std::pair<std::string, PythonObject>>& kwargs
        = std::vector<std::pair<std::string, PythonObject>>());

PythonObject callWithArgs(
    PythonObject from,
    const std::string& function,
    const std::vector<PythonObject>& args
        = std::vector<PythonObject>(),
    const std::vector<std::pair<std::string, PythonObject>>& kwargs
        = std::vector<std::pair<std::string, PythonObject>>());


} // end namespace wrappy

#include <wrappy/detail/call.hpp>
