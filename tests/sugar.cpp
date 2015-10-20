#define BOOST_TEST_MODULE sugar
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>

#include <wrappy/wrappy.h>

BOOST_AUTO_TEST_CASE(stdlib)
{
    auto datetime = wrappy::call("datetime.datetime", 2003, 8, 4, 12, 30, 45);
    auto formatted = wrappy::call(datetime, "isoformat");

    BOOST_CHECK_EQUAL(formatted.str(), "2003-08-04T12:30:45");
}

BOOST_AUTO_TEST_CASE(kwargs)
{
    auto delta = wrappy::call("datetime.timedelta", std::make_pair("hours", 1));
    auto seconds = delta.attr("seconds").num();

    BOOST_CHECK_EQUAL(seconds, 3600);
}
