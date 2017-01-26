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

PyObject *s_EmptyTuple;
PyObject *s_EmptyDict;

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

    s_EmptyTuple = Py_BuildValue("()");
    s_EmptyDict  = Py_BuildValue("{}");
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

// Load the longest prefix of name that is a valid module name.
// Returns null object if none is.
PythonObject loadModule(const std::string& name, size_t& dot)
{
    dot = name.size();
    PythonObject module;
    while (!module && dot != std::string::npos) {
        dot = name.rfind('.', dot-1);
        std::string prefix = name.substr(0, dot);
        module = PythonObject(PythonObject::owning {},
            PyImport_ImportModule(prefix.c_str()));
    }

    return module;
}

// name must start with a dot
PythonObject loadObject(PythonObject module, const std::string& name)
{
    // Evaluate the chain of dot-operators that leads from the module to
    // the function.
    PythonObject object = module;
    size_t suffixDot = 0;
    while(suffixDot != std::string::npos) {
        size_t next_dot = name.find('.', suffixDot+1);
        auto attr = name.substr(suffixDot+1, next_dot - (suffixDot+1));
        object = PythonObject(PythonObject::owning {}, PyObject_GetAttrString(object.get(), attr.c_str()));
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

PyObject* PythonObject::release()
{
    auto res = obj_;
    obj_ = nullptr;
    return res;
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

PythonObject PythonObject::operator()() const
{
    return PythonObject(owning{}, PyObject_Call(obj_, s_EmptyTuple, s_EmptyDict));
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

// Doesn't perform checks on the return value (input is still checked)
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
        throw WrappyError("Wrappy: Couldn't create python tuple.");
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

    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear(); // TODO add string to exception, make custom exception class
        throw WrappyError("Wrappy: Exception during call to python function");
    }

    if (!res) {
        throw WrappyError("Wrappy: Error calling function");
    }

    return res;
}

PythonObject load(
    const std::string& name)
{
    size_t cutoff;
    PythonObject module = loadModule(name, cutoff);
    PythonObject object;

    if (module) {
        object = loadObject(module, name.substr(cutoff));
    } else {
        // No proper prefix was a valid module, but maybe it's a built-in
        object = loadBuiltin(name);
    }

    if (!object) {
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

    return object;
}

PythonObject callWithArgs(
    const std::string& name,
    const std::vector<PythonObject>& args,
    const std::vector<std::pair<std::string, PythonObject>>& kwargs)
{
    PythonObject function = load(name);
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

    PythonObject function = loadObject(from, name);

    if (!function) {
        throw WrappyError("Wrappy: " 
            "Lookup of function " + functionName + " failed.");
    }

    return callFunctionWithArgs(function, args, kwargs);
}

//
// PythonIterator implementation
//

PythonIterator::PythonIterator(bool stopped, PythonObject iter):
  stopped_(stopped),
  iter_(iter)
{}

PythonIterator begin(PythonObject obj)
{
    PythonObject pyIter(PythonObject::owning{}, PyObject_GetIter(obj.get()));
    PythonIterator iter(false, pyIter);
    // Move iterator to first position in list to
    // initialize obj_
    return ++iter;
}

PythonIterator end(PythonObject)
{
    return PythonIterator(true, PythonObject());
}

PythonIterator& PythonIterator::operator++()
{
    auto next = iter_.attr("next"); // Change this to __next__ if switching to python 3

    // Can't use the normal "call" because we want to actually
    // handle the exception
    obj_ = PythonObject(PythonObject::owning{},
        PyObject_Call(next.get(), s_EmptyTuple, s_EmptyDict));

    if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_StopIteration) ) {
        stopped_ = true;
        PyErr_Clear();
    } else if (PyErr_Occurred() || !obj_) {
        PyErr_Print();
        PyErr_Clear();
        throw WrappyError("Unexcected exception during iteration");
    }

    return *this;
}

PythonObject PythonIterator::operator*()
{
    return obj_;
}

bool PythonIterator::operator!=(const PythonIterator& other) {
    return stopped_ != other.stopped_;
}


//
// wrapFunction implementation
//

namespace {

std::vector<PythonObject> to_vector(PyObject* pyargs)
{
    if (!PyTuple_Check(pyargs)) {
        throw WrappyError("Trampoling args was no tuple");
    }
    auto nargs = PyTuple_Size(pyargs);
    std::vector<PythonObject> args;
    args.reserve(nargs);
    for (ssize_t i=0; i<nargs; ++i) {
        args.emplace_back(PythonObject::borrowed{}, PyTuple_GetItem(pyargs, i));
    }
    return args;
}

std::map<const char*, PythonObject> to_map(PyObject* pykwargs)
{
    if (!PyDict_Check(pykwargs)) {
        throw WrappyError("Trampoling kwargs was no dict");
    }
    std::map<const char*, PythonObject> kwargs;
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(pykwargs, &pos, &key, &value)) {
        const char* str = PyString_AsString(key);
        PythonObject obj(PythonObject::borrowed{}, value);
        kwargs.emplace(str, obj);
    }

    return kwargs;
}

PyObject* trampolineWithData(PyObject* data, PyObject* pyargs, PyObject* pykwargs) {
    if (!PyCObject_Check(data)) {
        throw WrappyError("Trampoline data corrupted");
    }

    LambdaWithData fun = reinterpret_cast<LambdaWithData>(PyCObject_AsVoidPtr(data));
    void* userdata = PyCObject_GetDesc(data);
    auto args = to_vector(pyargs);
    auto kwargs = to_map(pykwargs);

    return fun(args, kwargs, userdata).release();
}

PyObject* trampolineNoData(PyObject* data, PyObject* pyargs, PyObject* pykwargs)
{
    if (!PyCObject_Check(data)) {
        throw WrappyError("Trampoline data corrupted");
    }

    Lambda fun = reinterpret_cast<Lambda>(PyCObject_AsVoidPtr(data));
    auto args = to_vector(pyargs);
    auto kwargs = to_map(pykwargs);

    return fun(args, kwargs).release();
}

// The reinterpret_cast<>'s here are technically undefined behaviour, but it's
// the only way that python's C API provides :(
PyMethodDef trampolineNoDataMethod {"trampoline1", reinterpret_cast<PyCFunction>(trampolineNoData), METH_KEYWORDS, nullptr};
PyMethodDef trampolineWithDataMethod {"trampoline2", reinterpret_cast<PyCFunction>(trampolineWithData), METH_KEYWORDS, nullptr};

} // end namespace

PythonObject construct(Lambda lambda)
{
    PyObject* pydata = PyCObject_FromVoidPtr(reinterpret_cast<void*>(lambda), nullptr);
    return PythonObject(PythonObject::owning{}, PyCFunction_New(&trampolineNoDataMethod, pydata));
}

PythonObject construct(LambdaWithData lambda, void* userdata)
{
    PyObject* pydata;
    if (!userdata) {
        pydata = PyCObject_FromVoidPtr(reinterpret_cast<void*>(lambda), nullptr);
    } else { // python returns an error if FromVoidPtrAndDesc is called with desc being null
        pydata = PyCObject_FromVoidPtrAndDesc(reinterpret_cast<void*>(lambda), userdata, nullptr);
    }
    return PythonObject(PythonObject::owning{}, PyCFunction_New(&trampolineWithDataMethod, pydata));
}


typedef PythonObject (*Lambda)(const std::vector<PythonObject>& args, const std::map<const char*, PythonObject>& kwargs);
typedef PythonObject (*LambdaWithData)(const std::vector<PythonObject>& args, const std::map<const char*, PythonObject>& kwargs, void* userdata);

} // end namespace wrappy
