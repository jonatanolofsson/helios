#include <os/utils/params.hpp>
#include <os/core/arguments.hpp>
#include <string>
#include <stdio.h>
#include <rapidjson/document.h>
#include <rapidjson/filestream.h>
#include <assert.h>
#include <iostream>

namespace os {
    Parameters parameters;

    void initParameters() {
        ArgumentDescription desc("Parameters");
        desc.add_options()
            ("params", arguments::value<std::string>()->default_value("params.json"), "Parameter file");

        Arguments args = getArguments(desc);
        std::string filename = args["params"].as<std::string>();
        FILE* pFile = fopen(filename.c_str(), "rb");
        rapidjson::FileStream is(pFile);
        parameters.ParseStream<0>(is);
        assert(!parameters.HasParseError());
        fclose(pFile);
    }
}
