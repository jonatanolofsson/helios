#include <os/core/arguments.hpp>
#include <os/utils/params.hpp>

namespace os {
    void init(int argc, char** argv) {
        registerArguments(argc, argv);
        initParameters();
    }
}
