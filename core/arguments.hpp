#pragma once
#include <boost/program_options.hpp>

namespace os {
    namespace arguments = boost::program_options;
    typedef arguments::options_description ArgumentDescription;
    typedef arguments::variables_map Arguments;

    void registerArguments(int argc, char** argv);
    Arguments getArguments(const ArgumentDescription& desc);
}
