#ifndef OS_TYPE_TRAITS_HPP_
#define OS_TYPE_TRAITS_HPP_

namespace os {
    //~ template<int N, typename THIS, typename Args...>    struct SelectType              : public SelectType<N-1, Args...> {} // Loop
    //~ template<typename THIS, typename Args...>           struct SelectType<0, THIS, Args...> { typedef THIS type; } // Select
    //~ template<int N, typename THIS>                      struct SelectType                   { typedef THIS type; } // Else

    template<typename T1, typename T2>  struct SameType         { static const bool value = false; };
    template<typename T1>               struct SameType<T1, T1> { static const bool value = true; };
}

#endif
