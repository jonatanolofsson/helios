#pragma once
#ifndef OS_COM_POSTOFFICE_HPP_
#define OS_COM_POSTOFFICE_HPP_
#include <os/com/SerialMessage.hpp>
#include <os/types.hpp>

namespace os {
    template<typename M>
    class PostOffice {
        public:
            struct Packager {
                typedef void(*Callback)(const U8*, const std::size_t);
                Callback callback;
                size_t alignment;
            };

            template<typename M::Id ID>
            void registerPackager(const typename Packager::Callback packagerCallback) {
                static_assert(ID < M::numberOfMessages, "Invalid ID");
                static_assert(alignof(typename M::template ById<ID>::Type) > 0, "Invalid alignment");
                packagers[ID] = {
                    packagerCallback,
                    alignof(typename M::template ById<ID>::Type)
                };
            }

        protected:
            std::size_t getAlignment(const U16 id) const {
                return packagers[id].alignment;
            }
            void dispatch(const U16 id, const U8* msg, const std::size_t len) const {
                //~ assert(packagers[id].alignment > 0);
                //~ BOOST_ASSERT((((std::size_t)msg % packagers[id].alignment) == 0));

                if(packagers[id].callback) {
                    packagers[id].callback(msg, len);
                } else {
                    /// \todo Warn about unhandled message type
                }
            }

        private:
            Packager packagers[M::numberOfMessages];
    };
}


#endif
