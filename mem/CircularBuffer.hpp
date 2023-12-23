#pragma once
#ifndef OS_MEM_CIRCULARBUFFER_HPP_
#define OS_MEM_CIRCULARBUFFER_HPP_

#include <os/mem/MultiCircularBuffer.hpp>

namespace os {

    template<typename T, int N>
    class CircularBuffer : public BufferSubCircle<T,N> {
        public:
            typedef MultiCircularBuffer<T,N> Buffer;
            typedef BufferSubCircle<T,N> SubCircle;

        private:
            Buffer buffer;

        public:
            CircularBuffer()
            {
                buffer.registerSubCircle(*dynamic_cast<SubCircle*>(this));
            }

            ~CircularBuffer()
            {
                buffer.unregisterSubCircle(*dynamic_cast<SubCircle*>(this));
            }


            /**
             * \brief   Reserves storage for future input to the buffer
             *
             * This function will not return until there is space available
             * to
             */
            T* reserve() {
                return buffer.reserve();
            }

            /**
             * \brief   Set the next input as ready
             */
            void push() {
                buffer.push();
            }

            /**
             * \brief   Set the next input as ready
             */
            void push(const T& v) {
                buffer.push(v);
            }

            /**
             * \brief   Prepare to die
             */
            void kill() {
                buffer.kill();
            }
    };
}

#endif
