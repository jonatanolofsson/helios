#include <os/utils/params.hpp>
#include <os/core/arguments.hpp>
#include <string>
#include <iostream>
#include <boost/property_tree/json_parser.hpp>

namespace os {
    Parameters parameters;
    namespace pt = boost::property_tree;

    void initParameters() {
        ArgumentDescription desc("Parameters");
        desc.add_options()
            ("params", arguments::value<std::string>()->default_value("params.json"), "Parameter file");

        Arguments args = getArguments(desc);
        std::string filename = args["params"].as<std::string>();

        std::ifstream buf(filename);
        pt::read_json(buf, parameters);
        buf.close();
    }
}
