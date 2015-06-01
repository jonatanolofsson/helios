#pragma once
#include <rapidjson/document.h>

namespace os {
    typedef rapidjson::Document Parameters;
    extern Parameters parameters;
    void initParameters();
}
