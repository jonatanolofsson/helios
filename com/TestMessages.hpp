#ifndef TESTMESSAGES_HPP_
#define TESTMESSAGES_HPP_

#include <os/types.hpp>

namespace os {
    template<int ID> struct TestMessage;
    struct TestMessages {
        static const int numberOfMessages = 3;

        enum Id {
            myTestMessage0 = 0,
            myTestMessage1 = 1,
            myTestMessage2 = 2
        };
        template<Id MID>
        struct Message { typedef typename TestMessage<MID>::Type Type; };
    };

    struct MyTestMessage0 {
        static const TestMessages::Id ID = TestMessages::myTestMessage0;
        S32 a;
    };
    struct MyTestMessage1 {
        static const TestMessages::Id ID = TestMessages::myTestMessage1;
        U32 a;
        U8 b;
    };
    struct MyTestMessage2 {
        static const TestMessages::Id ID = TestMessages::myTestMessage2;
        S32 a;
    };

    template<> struct TestMessage<TestMessages::Id::myTestMessage0> { typedef MyTestMessage0 Type; };
    template<> struct TestMessage<TestMessages::Id::myTestMessage1> { typedef MyTestMessage1 Type; };
    template<> struct TestMessage<TestMessages::Id::myTestMessage2> { typedef MyTestMessage2 Type; };
}
#endif
