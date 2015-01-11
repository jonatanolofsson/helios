#pragma once
#include <boost/property_tree/ptree.hpp>

namespace os {
    typedef boost::property_tree::ptree Parameters;
    extern Parameters parameters;
    void initParameters();
}
