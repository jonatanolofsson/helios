#pragma once
#ifndef OS_MEM_MULTICIRCULARBUFFER_HPP_
#define OS_MEM_MULTICIRCULARBUFFER_HPP_

#include <mutex>
#include <condition_variable>
#include <os/exceptions.hpp>
#include <assert.h>
#include <os/core/Semaphore.hpp>
#include <os/utils/eventlog.hpp>

#include <iostream>

namespace os {
    template<typename T, int N> class BufferSubCircle;
    template<typename T, int N>
    class MultiCircularBuffer {
        public:
            typedef MultiCircularBuffer<T,N> Self;
            struct Index {
                private:
                    int i;
                public:
                    explicit Index(const int i_ = 0) : i(i_%N) {}
                    bool operator==(const Index& that) const {
                        return that.i == i;
                    }
                    bool operator==(const int& that) const {
                        return that == i;
                    }
                    int operator+(const int a) const {
                        return (i+a)%N;
                    }
                    int operator++() {
                        i = (i+1)%N;
                        return i;
                    }
                    int index() const {
                        return i;
                    }
            };

            typedef BufferSubCircle<T,N> SubCircle;

        private:
            Index iPointer, oPointer_int;
            T storage[N];
            unsigned int circleCounter[N];
            unsigned int nofCircles;
            std::mutex inputGuard, counterGuard;
            std::condition_variable spaceAvailable, outputAvailable;
            os::Semaphore pleaseDontDie;
            bool dying;

            /**
             * \brief   Check if the buffer is empty
             */
            bool empty_safe(const Index& oPointer) {
                return (oPointer == iPointer);
            }

        public:
            MultiCircularBuffer()
            : iPointer(0)
            , oPointer_int(0)
            , nofCircles(0)
            , pleaseDontDie(1)
            , dying(false) {
                //LOG_EVENT(typeid(Self).name(), 0, "Created");
            }
            ~MultiCircularBuffer() {
                //LOG_EVENT(typeid(Self).name(), 0, "Dying");
                kill();
                //LOG_EVENT(typeid(Self).name(), 0, "Died");
            }

            void registerSubCircle(SubCircle& c) {
                //~ std::cout << "Register sc" << std::endl;
                std::unique_lock<std::mutex> l(counterGuard);
                ++nofCircles;
                for(unsigned i = 0; i < N; ++i) {
                    ++circleCounter[i];
                }
                c.setBuffer(this, &iPointer);
            }
            void unregisterSubCircle(SubCircle& c) {
                //~ std::cout << "Unregister sc" << std::endl;
                std::unique_lock<std::mutex> l(counterGuard);
                --nofCircles;
                for(unsigned i = 0; i < N; ++i) {
                    --circleCounter[i];
                }
                c.setBuffer(nullptr);
            }

            void kill() {
                if(dying) return;
                dying = true;
                notify_all();
                //LOG_EVENT(typeid(Self).name(), 0, "Going down: " << pleaseDontDie.value());
                pleaseDontDie.down();
            }
            /**
             * \brief   Reserves storage for future input to the buffer
             *
             * This function will not return until there is space available
             * to
             */
            T* reserve() {
                os::SemaphoreGuard s(pleaseDontDie);
                std::unique_lock<std::mutex> l(counterGuard);
                while(!dying && oPointer_int == (iPointer+1)) {
                    LOG_EVENT(typeid(T).name(), 0, "Buffer full");
                    spaceAvailable.wait(l);
                    LOG_EVENT(typeid(T).name(), 0, "Bufferspace available again");
                }
                if(dying) return nullptr;
                //~ std::cout << "Reserved space in queue" << std::endl;
                inputGuard.lock();
                return &storage[iPointer.index()];
            }

            /**
             * \brief   Set the next input as ready
             */
            void push() {
                {
                    std::unique_lock<std::mutex> l(counterGuard);
                    //~ std::cout << "Push to queue " << typeid(T).name() << std::endl;
                    circleCounter[iPointer.index()] = nofCircles;
                    ++iPointer;
                    inputGuard.unlock();
                }
                outputAvailable.notify_all();
                //~ std::cout << "Pushed to queue " << typeid(T).name() << std::endl;
            }

            /**
             * \brief   Set the next input as ready
             */
            void push(const T& v) {
                *reserve() = v;
                push();
            }

            /**
             * \brief   Return pointer to the next free unit in the buffer.
             */
            T* next(const Index& oPointer, volatile bool*const bailout = nullptr) {
                os::SemaphoreGuard s(pleaseDontDie);
                std::unique_lock<std::mutex> l(counterGuard);
                //~ std::cout << "Getting next in queue " << typeid(T).name() << std::endl;
                while(!dying && empty_safe(oPointer) && !(bailout && *bailout)) outputAvailable.wait(l);
                if(bailout && *bailout) {
                    LOG_EVENT(typeid(Self).name(), 0, "Bailing out");
                    throw os::HaltException();
                }
                if(dying) return nullptr;
                //~ std::cout << "Got next in queue" << std::endl;
                return &storage[oPointer.index()];
            }

            /**
             * \brief   Return the first unit in the buffer.
             */
            T popNextValue(Index& oPointer, volatile bool*const bailout) {
                auto v = next(oPointer, bailout);
                if(nullptr == v) {
                    LOG_EVENT(typeid(Self).name(), 0, "Popped nullptr");
                    throw os::HaltException();
                }
                pop(oPointer);
                return *v;
            }

            /**
             * \brief   Return pointer to the first unit in the buffer, or nullptr if the buffer is empty.
             */
            T* nextOrNot(const Index& oPointer) {
                std::unique_lock<std::mutex> l(counterGuard);
                return (empty_safe(oPointer) ? nullptr : &storage[oPointer.index()]);
            }

            /**
             * \brief   Flag a unit as ready for re-use
             */
            void pop(Index& oPointer) {
                std::unique_lock<std::mutex> l(counterGuard);
                if(!empty_safe(oPointer)) {
                    //LOG_EVENT(typeid(Self).name(), 0, "Popping from queue, " << circleCounter[oPointer.index()] << " subcircles.");
                    --circleCounter[oPointer.index()];
                    if(0 == circleCounter[oPointer.index()]) {
                        //LOG_EVENT(typeid(Self).name(), 0, "Last popper");
                        //~ std::cout << "Popped from queue" << std::endl;
                        ++oPointer_int;
                        spaceAvailable.notify_all();
                    }
                    ++oPointer;
                } else {
                    LOG_EVENT(typeid(Self).name(), 0, "Tried to pop from empty queue");
                }
            }

            /**
             * \brief   Check if the buffer is empty
             */
            bool empty(const Index& oPointer) {
                std::unique_lock<std::mutex> l(counterGuard);
                return (oPointer == iPointer);
            }

            /**
             * \brief   Wake up all waiting threads e.g. to check for bailout
             */
            void notify_all() {
                outputAvailable.notify_all();
                spaceAvailable.notify_all();
            }
    };


    template<typename T, int N>
    class BufferSubCircle {
        public:
            typedef MultiCircularBuffer<T,N> Buffer;
            typedef typename Buffer::Index Index;

        private:
            Buffer* o;
            Index oPointer;

            void setBuffer(Buffer* const b, const Index*const v = nullptr) {
                o = b;
                if(v) oPointer = *v;
            }

            friend Buffer;

        public:
            BufferSubCircle() : o(nullptr) {}

            /**
             * \brief   Return pointer to the next free unit in the buffer.
             */
            T* next(volatile bool*const bailout = nullptr) {
                assert(o);
                return o->next(oPointer, bailout);
            }

            /**
             * \brief   Return the first unit in the buffer.
             */
            T nextValue() {
                assert(o);
                return o->nextValue(oPointer);
            }

            /**
             * \brief   Return the first unit in the buffer.
             */
            T popNextValue(volatile bool*const bailout) {
                assert(o);
                return o->popNextValue(oPointer, bailout);
            }

            /**
             * \brief   Return pointer to the first unit in the buffer, or nullptr if the buffer is empty.
             */
            T* nextOrNot() {
                assert(o);
                return o->nextOrNot(oPointer);
            }

            /**
             * \brief   Flag a unit as ready for re-use
             */
            void pop() {
                assert(o);
                o->pop(oPointer);
            }

            /**
             * \brief   Check if the buffer is empty
             */
            bool empty() const {
                assert(o);
                return o->empty(oPointer);
            }
    };
}

#endif
