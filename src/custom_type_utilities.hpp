#pragma once

#include <type_traits>

template <typename T, typename... Types>
struct is_type_among
{
  static constexpr bool value = (std::is_same_v<T, Types> || ...);
};

template <typename T, typename... Types>
constexpr bool is_type_among_v = is_type_among<T, Types...>::value;

template <int N, typename... T>
struct nth_type_of;

template <typename T0, typename... Types>
struct nth_type_of<0, T0, Types...>
{
  typedef T0 type;
};
template <int N, typename T0, typename... Types>
struct nth_type_of<N, T0, Types...>
{
  typedef typename nth_type_of<N - 1, Types...>::type type;
};

template <int N, typename... Types>
using nth_type_of_t = typename nth_type_of<N, Types...>::type;
