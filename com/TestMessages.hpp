#pragma once
#ifndef TESTMESSAGES_HPP_
#define TESTMESSAGES_HPP_

#include <os/types.hpp>

namespace os {
    namespace testmessages {
        template<int ID> struct Message;
        struct Messages {
            static const int numberOfMessages = 3;

            enum Id {
                myTestMessage0 = 0,
                myTestMessage1 = 1,
                myTestMessage2 = 2
            };
            template<Id MID>
            struct ById { typedef typename Message<MID>::Type Type; };
        };

        struct MyTestMessage0 {
            static const Messages::Id ID = Messages::myTestMessage0;
            S32 a;
        };
        struct MyTestMessage1 {
            static const Messages::Id ID = Messages::myTestMessage1;
            U32 a;
            U8 b;
        };
        struct MyTestMessage2 {
            static const Messages::Id ID = Messages::myTestMessage2;
            S32 a;
        };

        template<> struct Message<Messages::Id::myTestMessage0> { typedef MyTestMessage0 Type; };
        template<> struct Message<Messages::Id::myTestMessage1> { typedef MyTestMessage1 Type; };
        template<> struct Message<Messages::Id::myTestMessage2> { typedef MyTestMessage2 Type; };
    }
}
#endif
