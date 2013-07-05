add_library(utils
    pty.cpp
    sprintf.cpp
)

add_library(os_utils OBJECT eventlog.cpp)

add_testdirectory(tests)
