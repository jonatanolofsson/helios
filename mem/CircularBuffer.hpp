#pragma once
#ifndef OS_MEM_CIRCULARBUFFER_HPP_
#define OS_MEM_CIRCULARBUFFER_HPP_

#include <mutex>
#include <condition_variable>
#include <os/exceptions.hpp>

#include <iostream>

namespace os {
    template<typename T, int N>
    class CircularBuffer {
        private:
            struct Index {
                private:
                    int i;
                public:
                    Index(const int i_) : i(i_%N) {}
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
            } iPointer, oPointer;
            T storage[N];
            std::mutex inputGuard, counterGuard;
            std::condition_variable icond, ocond;
            bool dying;

        public:
            CircularBuffer() : iPointer(0), oPointer(0), dying(false) {}
            ~CircularBuffer() {
                kill();
            }

            void kill() {
                dying = true;
                icond.notify_all();
                ocond.notify_all();
            }
            /**
             * \brief   Reserves storage for future input to the buffer
             *
             * This function will not return until there is space available
             * to
             */
            T* reserve() {
                std::unique_lock<std::mutex> l(counterGuard);
                while(!dying && oPointer == (iPointer+1)) {
                    //~ std::cout << "Buffer full.." << std::endl;
                    icond.wait(l);
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
                std::unique_lock<std::mutex> l(counterGuard);
                //~ std::cout << "Push to queue" << std::endl;
                ++iPointer;
                inputGuard.unlock();
                ocond.notify_all();
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
            T* next() {
                std::unique_lock<std::mutex> l(counterGuard);
                while(!dying && empty()) ocond.wait(l);
                if(dying) return nullptr;
                //~ std::cout << "Got next in queue" << std::endl;
                return &storage[oPointer.index()];
            }

            /**
             * \brief   Return pointer to the next free unit in the buffer.
             */
            T* next(volatile bool& bailout) {
                std::unique_lock<std::mutex> l(counterGuard);
                while(!dying && empty() && !bailout) ocond.wait(l);
                if(bailout) throw os::HaltException();
                if(dying) return nullptr;
                //~ std::cout << "Got next in queue" << std::endl;
                return &storage[oPointer.index()];
            }

            /**
             * \brief   Return the first unit in the buffer.
             */
            T nextValue() {
                return *next();
            }

            /**
             * \brief   Return the first unit in the buffer.
             */
            T popNextValue(volatile bool& bailout) {
                auto v = next(bailout);
                if(nullptr == v) {
                    throw os::HaltException();
                }
                pop();
                return *v;
            }

            /**
             * \brief   Return pointer to the first unit in the buffer, or nullptr if the buffer is empty.
             */
            T* nextOrNot() {
                std::unique_lock<std::mutex> l(counterGuard);
                return (empty() ? nullptr : &storage[oPointer.index()]);
            }

            /**
             * \brief   Flag a unit as ready for re-use
             */
            void pop() {
                std::unique_lock<std::mutex> l(counterGuard);
                if(!empty()) {
                    //~ std::cout << "Popped from queue" << std::endl;
                    ++oPointer;
                    icond.notify_all();
                }
            }

            /**
             * \brief   Check if the buffer is empty
             */
            bool empty() const {
                return (oPointer == iPointer);
            }

            /**
             * \brief   Wake up all waiting threads e.g. to check for bailout
             */
            void notify_all() {
                ocond.notify_all();
                icond.notify_all();
            }
    };
}

#endif
