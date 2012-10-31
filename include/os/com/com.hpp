#ifndef OS_COM_COM_HPP_
#define OS_COM_COM_HPP_

namespace os {
    namespace com {
        #ifndef MAX_EXECUTOR_PARAMS
            #define MAX_EXECUTOR_PARAMS 10
        #endif

        template<typename T> void yield(const T&);
    }
}

#endif
