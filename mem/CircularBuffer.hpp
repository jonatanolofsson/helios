#pragma once
#ifndef OS_MEM_CIRCULARBUFFER_HPP_
#define OS_MEM_CIRCULARBUFFER_HPP_

#include <mutex>
#include <condition_variable>

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
                while(!dying && oPointer == (iPointer+1)) icond.wait(l);
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
             * \brief   Return pointer to the first unit in the buffer.
             */
            T* next() {
                std::unique_lock<std::mutex> l(counterGuard);
                while(!dying && oPointer == iPointer) ocond.wait(l);
                if(dying) return nullptr;
                //~ std::cout << "Got next in queue" << std::endl;
                return &storage[oPointer.index()];
            }

            /**
             * \brief   Return pointer to the first unit in the buffer, or nullptr if the buffer is empty.
             */
            T* nextOrNot() {
                std::unique_lock<std::mutex> l(counterGuard);
                if(oPointer == iPointer) {
                    return nullptr;
                }
                while(!dying && oPointer == iPointer) ocond.wait(l);
                if(dying) return nullptr;
                //~ std::cout << "Got next in queue" << std::endl;
                return &storage[oPointer.index()];
            }

            /**
             * \brief   Flag a unit as ready for re-use
             */
            void pop() {
                std::unique_lock<std::mutex> l(counterGuard);
                if(!(oPointer == iPointer)) {
                    //~ std::cout << "Pop from queue" << std::endl;
                    ++oPointer;
                    icond.notify_all();
                }
            }
    };
}

#endif
