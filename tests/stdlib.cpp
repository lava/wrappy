#define BOOST_TEST_MODULE stdlib
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <wrappy/wrappy.h>

#include <vector>
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE(random_number)
{
    std::vector<wrappy::PythonObject> args;
    args.push_back(wrappy::construct(0));

    wrappy::callWithArgs("random.seed", args);
    auto v1 = wrappy::callWithArgs("random.random");

    wrappy::callWithArgs("random.seed", args);
    auto v2 = wrappy::callWithArgs("random.random");

    BOOST_CHECK_EQUAL(v1.num(), v2.num());
}

BOOST_AUTO_TEST_CASE(builtins)
{
    std::vector<wrappy::PythonObject> args(1);

    args[0] = wrappy::construct(255);
    auto intval = wrappy::callWithArgs("hex", args);

    args[0] = wrappy::construct(255ll);
    auto longval = wrappy::callWithArgs("hex", args);

    BOOST_CHECK_EQUAL(intval.str(), "0xff");
    BOOST_CHECK_EQUAL(longval.str(), "0xffL");
}

BOOST_AUTO_TEST_CASE(error)
{
    BOOST_CHECK_THROW(wrappy::callWithArgs("asdf"),
        wrappy::WrappyError);
}

BOOST_AUTO_TEST_CASE(destruction)
{

    {
        auto v1 = wrappy::callWithArgs("random.random");
    }

    {
        auto v2 = wrappy::callWithArgs("random.random");
    }

    // test successful if python didn't crash
}
