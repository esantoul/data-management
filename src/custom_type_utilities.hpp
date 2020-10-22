/**
 * Copyright (C) 2020 Etienne Santoul - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the BSD 2-Clause License
 *
 * You should have received a copy of the BSD 2-Clause License
 * with this file. If not, please visit: 
 * https://github.com/esantoul/data-management
 */

#pragma once

#include <type_traits>
#include <cstddef>

namespace dmgmt
{
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

  template <typename T, typename... Types>
  struct IndexLocator;

  template <typename T>
  struct IndexLocator<T>
  {
    static constexpr std::size_t locate() { return 0; }
  };

  template <typename T, typename T0, typename... Types>
  struct IndexLocator<T, T0, Types...>
  {
    static constexpr std::size_t locate()
    {
      if (std::is_same_v<T, T0>)
        return 0;
      else
        return 1 + IndexLocator<T, Types...>::locate();
    }
  };

  template <typename T, typename... Types>
  struct get_type_index
  {
    static constexpr std::enable_if_t<is_type_among_v<T, Types...>, size_t> value = IndexLocator<T, Types...>::locate();
  };

  template <typename T, typename... Types>
  constexpr std::size_t get_type_index_v = get_type_index<T, Types...>::value;

  template <typename T, typename EqualTo>
  struct has_operator_equal_impl
  {
    template <typename U, typename V>
    static auto test(U *) -> decltype(std::declval<const U>() == std::declval<const V>());
    template <typename, typename>
    static auto test(...) -> std::false_type;

    using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
  };

  template <typename T, typename EqualTo = T>
  struct has_operator_equal : has_operator_equal_impl<T, EqualTo>::type
  {
  };

  template <typename T, typename EqualTo = T>
  constexpr bool has_operator_equal_v = has_operator_equal<T, EqualTo>::value;
} // namespace dmgmt