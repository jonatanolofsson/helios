#include <os/core/arguments.hpp>

namespace os {
    int argc;
    char** argv;

    void registerArguments(int argc_, char** argv_) {
        argc = argc_;
        argv = argv_;
    }

    Arguments getArguments(const ArgumentDescription& desc) {
        Arguments args;
        arguments::store(arguments::parse_command_line(argc, argv, desc), args);
        arguments::notify(args);
        return args;
    }
}
