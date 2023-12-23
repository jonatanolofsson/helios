#include <os/utils/params.hpp>
#include <os/core/arguments.hpp>
#include <string>
#include <stdio.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <assert.h>
#include <fstream>

namespace os {
    Parameters parameters;

    void initParameters() {
        ArgumentDescription desc("Parameters");
        desc.add_options()
            ("params", arguments::value<std::string>()->default_value("params.json"), "Parameter file");

        Arguments args = getArguments(desc);
        std::string filename = args["params"].as<std::string>();
        std::ifstream ifs(filename);
        rapidjson::IStreamWrapper isw(ifs);
        parameters.ParseStream<0>(isw);
        assert(!parameters.HasParseError());
    }
}
