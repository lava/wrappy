// Python header must be included first since they insist on 
// unconditionally defining some system macros
// (http://bugs.python.org/issue1045893, still broken in python3.4)
#include <Python.h>

#include <wrappy/wrappy.h>

#include <iostream>
#include <mutex>
#include <cstdio>

namespace wrappy {

PythonObject None, True, False;

} // end namespace wrappy

namespace {

using namespace wrappy;

__attribute__((constructor))
void wrappyInitialize()
{
    // Initialize python interpreter.
    // The module search path is initialized as following:
    // Python looks at PATH to find an executable called "python"
    // The name that is searched can be changed by calling Py_SetProgramName()
    // before Py_Initialize(). The folder where this executable resides is
    // python-home, which can be overwritten at runtime by setting $PYTHONHOME.
    // The default module search path is then
    //
    //     <python-home>/../lib/<python-version>/
    //
    // All entries from $PYTHONPATH are pre-pended to the module search path

    Py_Initialize();
    // Setting a dummy value since many libraries require sys.argv[0] to exist
    char* dummy_args[] = {const_cast<char*>("wrappy"), nullptr};
    PySys_SetArgvEx(1, dummy_args, 0);

    wrappy::None  = PythonObject(PythonObject::borrowed{}, Py_None);
    wrappy::True  = PythonObject(PythonObject::borrowed{}, Py_True);
    wrappy::False = PythonObject(PythonObject::borrowed{}, Py_False);
}


__attribute__((destructor))
void wrappyFinalize()
{
    Py_Finalize();
}

PythonObject loadBuiltin(const std::string& name)
{
    auto builtins = PyEval_GetBuiltins(); // returns a borrowed reference
    PythonObject function(PythonObject::borrowed {},
        PyDict_GetItemString(builtins, name.c_str()));

    return function;
}

// Load the longest prefix of name that is a valid module name
// Returns null object if 
PythonObject loadModule(const std::string& name, size_t& dot)
{
    dot = name.size();
    PythonObject module;
    while (!module && dot != std::string::npos) {
        dot = name.rfind('.', dot-1);
        auto prefix = name.substr(0, dot).c_str();
        module = PythonObject(PythonObject::owning {},
            PyImport_ImportModule(prefix));
    }

    return module;
}

// name must start with a dot
PythonObject loadFunction(PythonObject module, const std::string& name)
{
    // Evaluate the chain of dot-operators that leads from the module to
    // the function.
    PythonObject object = module;
    size_t suffixDot = 0;
    while(suffixDot != std::string::npos) {
        size_t next_dot = name.find('.', suffixDot+1);
        auto attr = name.substr(suffixDot+1, next_dot-(suffixDot+1)).c_str();
        object = PythonObject(PythonObject::owning {}, PyObject_GetAttrString(object.get(), attr));
        suffixDot = next_dot;
    }

    return object;
}

} // end unnamed namespace

namespace wrappy {

PythonObject::PythonObject()
    : obj_(nullptr)
{ }

PythonObject::PythonObject(owning, PyObject* value)
    : obj_(value)
{ }

PythonObject::PythonObject(borrowed, PyObject* value)
    : obj_(value)
{
    Py_XINCREF(obj_);
}

PythonObject::~PythonObject()
{
    Py_XDECREF(obj_);
}

PythonObject::PythonObject(const PythonObject& other)
    : obj_(other.obj_)
{
    Py_XINCREF(obj_);
}

PythonObject& PythonObject::operator=(const PythonObject& other)
{
    PythonObject tmp(other);
    std::swap(obj_, tmp.obj_);

    return *this;
}

PythonObject::PythonObject(PythonObject&& other)
    : obj_(nullptr)
{
    std::swap(obj_, other.obj_);
}

PythonObject& PythonObject::operator=(PythonObject&& other)
{
    std::swap(obj_, other.obj_);

    return *this;
}

PyObject* PythonObject::get() const
{
    return obj_;
}

PythonObject PythonObject::attr(const std::string& name) const
{
    return PythonObject(owning{}, PyObject_GetAttrString(obj_, name.c_str()));
}

long long PythonObject::num() const
{
    return PyLong_AsLongLong(obj_);
}

double PythonObject::floating() const
{
    return PyFloat_AsDouble(obj_);
}

const char* PythonObject::str() const
{
    return PyString_AsString(obj_);
}

PythonObject::operator bool() const
{
    return obj_ != nullptr;
}

PythonObject construct(long long ll)
{
    return PythonObject(PythonObject::owning {}, PyLong_FromLongLong(ll));
}

PythonObject construct(int i)
{
    return PythonObject(PythonObject::owning {}, PyInt_FromLong(i));
}

PythonObject construct(double d)
{
    return PythonObject(PythonObject::owning {}, PyFloat_FromDouble(d));
}

PythonObject construct(const std::string& str)
{
    return PythonObject(PythonObject::owning {}, PyString_FromString(str.c_str()));
}

PythonObject construct(const std::vector<PythonObject>& v)
{
    PythonObject list(PythonObject::owning {}, PyList_New(v.size()));
    for (size_t i = 0; i < v.size(); ++i) {
        PyObject* item = v.at(i).get();
        Py_XINCREF(item); // PyList_SetItem steals a reference
        PyList_SetItem(list.get(), i, item);
    }
    return list;
}

PythonObject construct(PythonObject object)
{
    return object;
}

void addModuleSearchPath(const std::string& path)
{
    std::string pathString("path");
    auto syspath = PySys_GetObject(&pathString[0]); // Borrowed reference

    PythonObject pypath(PythonObject::owning {},
        PyString_FromString(path.c_str()));

    if (!pypath) {
        throw WrappyError("Wrappy: Can't allocate memory for string.");
    }

    auto pos = PyList_Insert(syspath, 0, pypath.get());
    if (pos < 0) {
        throw WrappyError("Wrappy: Couldn't add " + path + " to sys.path");
    }
}

PythonObject callFunctionWithArgs(
    PythonObject function,
    const std::vector<PythonObject>& args,
    const std::vector<std::pair<std::string, PythonObject>>& kwargs)
{
    if (!PyCallable_Check(function.get())) {
        throw WrappyError("Wrappy: Supplied object isn't callable.");
    }

    // Build tuple
    size_t sz = args.size();
    PythonObject tuple(PythonObject::owning {}, PyTuple_New(sz));
    if (!tuple) {
        PyErr_Print();
        throw WrappyError("Wrappy: Couldn't create python typle.");
    }

    for (size_t i = 0; i < sz; ++i) {
        PyObject* arg = args.at(i).get();
        Py_XINCREF(arg); // PyTuple_SetItem steals a reference
        PyTuple_SetItem(tuple.get(), i, arg);
    }

    // Build kwargs dict
    PythonObject dict(PythonObject::owning {}, PyDict_New());
    if (!dict) {
        PyErr_Print();
        throw WrappyError("Wrappy: Couldn't create python dictionary.");
    }

    for (const auto& kv : kwargs) {
        PyDict_SetItemString(dict.get(), kv.first.c_str(), kv.second.get());
    }

    PythonObject res(PythonObject::owning{},
        PyObject_Call(function.get(), tuple.get(), dict.get()));

    if (!res) {
        PyErr_Print();
        throw WrappyError("Wrappy: Error calling function");
    }

    return res;
}

PythonObject callWithArgs(
    const std::string& name,
    const std::vector<PythonObject>& args,
    const std::vector<std::pair<std::string, PythonObject>>& kwargs)
{
    size_t cutoff;
    PythonObject module = loadModule(name, cutoff);
    PythonObject function;

    if (module) {
        function = loadFunction(module, name.substr(cutoff));
    } else {
        // No proper prefix was a valid module, but maybe it's a built-in
        function = loadBuiltin(name);
    }

    if (!function) {
        std::string error_message;
        if(cutoff != std::string::npos) {
            error_message = "Wrappy: Lookup of function " +
                name.substr(cutoff) + " in module " +
                name.substr(0,cutoff) + " failed.";
        } else {
            error_message = "Wrappy: Lookup of function " + name + "failed.";
        }

        throw WrappyError(error_message);
    }

    return callFunctionWithArgs(function, args, kwargs);
}

// Call a python function with arguments args and keyword arguments kwargs
PythonObject callWithArgs(
    PythonObject from,
    const std::string& functionName,
    const std::vector<PythonObject>& args,
    const std::vector<std::pair<std::string, PythonObject>>& kwargs)
{
    std::string name;
    if (functionName[0] == '.') {
        name = functionName;
    } else {
        name = "." + functionName;
    }

    PythonObject function = loadFunction(from, name);

    if (!function) {
        throw WrappyError("Wrappy: " 
            "Lookup of function " + functionName + " failed.");
    }

    return callFunctionWithArgs(function, args, kwargs);
}

} // end namespace wrappy
