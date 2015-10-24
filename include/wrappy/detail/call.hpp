#pragma once

// Implementation of call()
namespace wrappy {
namespace detail {

typedef std::vector<PythonObject> PositionalArgs;
typedef std::vector<std::pair<std::string, PythonObject>> KeywordArgs;

template<typename... Args>
void appendArgs(PositionalArgs& pargs, KeywordArgs& kwargs, Args... args);

// Base case
template<>
inline void appendArgs(PositionalArgs&, KeywordArgs&)
{}

// Positional argument
template<typename Head, typename... Tail>
void appendArgs(
    PositionalArgs& pargs,
    KeywordArgs& kwargs,
    Head head,
    Tail... tail)
{
    pargs.push_back(construct(head));    
    appendArgs(pargs, kwargs, tail...);
}

// Keyword argument from string
template<typename Head, typename... Tail>
void appendArgs(
    PositionalArgs& pargs,
    KeywordArgs& kwargs,
    std::pair<std::string, Head> head,
    Tail... tail)
{
    kwargs.emplace_back(head.first, construct(head.second));
    appendArgs(pargs, kwargs, tail...);
}

// Keyword argument from const char*
template<typename Head, typename... Tail>
void appendArgs(
    PositionalArgs& pargs,
    KeywordArgs& kwargs,
    std::pair<const char*, Head> head,
    Tail... tail)
{
    kwargs.emplace_back(std::string(head.first), construct(head.second));
    appendArgs(pargs, kwargs, tail...);
}

} // end namespace detail

template<typename... Args>
PythonObject call(const std::string& f, Args... args)
{
    auto pargs = std::vector<PythonObject>();
    auto kwargs = std::vector<std::pair<std::string, PythonObject>>();
    detail::appendArgs(pargs, kwargs, args...);
    return callWithArgs(f, pargs, kwargs);
}

template<typename... Args>
PythonObject call(PythonObject from, const std::string& f, Args... args)
{
    auto pargs = std::vector<PythonObject>();
    auto kwargs = std::vector<std::pair<std::string, PythonObject>>();
    detail::appendArgs(pargs, kwargs, args...);
    return callWithArgs(from, f, pargs, kwargs);
}

template<typename... Args>
PythonObject PythonObject::call(const std::string& f, Args... args)
{
    return wrappy::call(*this, f, args...);
}

} // end namespace wrappy

