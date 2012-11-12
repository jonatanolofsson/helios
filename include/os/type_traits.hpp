#ifndef OS_TYPE_TRAITS_HPP_
#define OS_TYPE_TRAITS_HPP_

namespace os {
    template<int N, typename THIS, typename Args...> struct SelectType              : public SelectType<N-1, Args...> {} // Loop
    template<typename THIS, typename Args...>   struct SelectType<0, THIS, Args...> { typedef THIS RET; } // Select
    template<int N, typename THIS>              struct SelectType                   { typedef THIS RET; } // Else
}

#endif
