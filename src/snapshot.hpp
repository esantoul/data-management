#pragma once

#include <memory>
#include <variant>

#include "data_signature.hpp"
#include "custom_type_utilities.hpp"

/**
 * @brief Stores a variable DataSignature and a copy of its value at a point in time
 * @tparam Types List of all the types that are allowed be stored in the element
 */
template <typename... Types>
class Snapshot
{
private:
  using v_t = std::variant<typename std::unique_ptr<Types>...>;

public:
  template <typename El_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
  constexpr Snapshot(El_t &element) noexcept
      : address{reinterpret_cast<void *>(&element)},
        data{std::make_unique<El_t>(element)}
  {
  }

  constexpr Snapshot(const Snapshot &other) noexcept
      : address{other.address}
  {
    CopyHelper<sizeof...(Types) - 1, Types...>::attempt(data, other.data);
  }

  template <typename T>
  constexpr bool operator==(const T &val) const noexcept
  {
    // Check whether data currently holds a std::unique_ptr to T
    if (data.index() != get_type_index_v<T, Types...>)
      return false;
    // Check whether address and val address are the same
    if (static_cast<const void *>(&val) != address)
      return false;
    // Check whether data holds the same value as val
    return ((std::is_same_v<T, Types> && EqualHelper<Types, T>::test(data, val)) || ...);
  }

  template <typename T>
  constexpr bool operator!=(const T &val) const noexcept
  {
    return !(val == *this);
  }

  constexpr void rollback() const noexcept
  {
    // using fold expression & binary operators short-circuiting
    // the void is here to avoid having a unused variable warning
    void(((data.index() == get_type_index_v<Types, Types...> &&
           (bool)&(*reinterpret_cast<Types *>(address) = *std::get<typename std::unique_ptr<Types>>(data).get())) ||
          ...));
  }

  constexpr DataSignature get_data_signature() const noexcept
  {
    return DataSignatureHelper<sizeof...(Types) - 1, Types...>::attempt(address, data);
  }

private:
  template <size_t N, typename... Args>
  struct CopyHelper
  {
    static constexpr void attempt(std::variant<typename std::unique_ptr<Args>...> &dest, const std::variant<typename std::unique_ptr<Args>...> &source) noexcept
    {
      if (source.index() == N)
        dest = std::make_unique<nth_type_of_t<N, Args...>>(*std::get<N>(source).get());
      else
        CopyHelper<N - 1, Args...>::attempt(dest, source);
    }
  };

  template <typename... Args>
  struct CopyHelper<0, Args...>
  {
    static constexpr void attempt(std::variant<typename std::unique_ptr<Args>...> &dest, const std::variant<typename std::unique_ptr<Args>...> &source) noexcept
    {
      if (source.index() == 0)
        dest = std::make_unique<nth_type_of_t<0, Args...>>(*std::get<0>(source).get());
    }
  };

  template <size_t N, typename... Args>
  struct DataSignatureHelper
  {
    static constexpr DataSignature attempt(const void *a, const std::variant<typename std::unique_ptr<Args>...> &d) noexcept
    {
      if (d.index() == N)
        return *reinterpret_cast<const nth_type_of_t<N, Args...> *>(a);
      else
        return DataSignatureHelper<N - 1, Args...>::attempt(a, d);
    }
  };

  template <typename... Args>
  struct DataSignatureHelper<0, Args...>
  {
    static constexpr DataSignature attempt(const void *a, const std::variant<typename std::unique_ptr<Args>...> &d) noexcept
    {
      if (d.index() == 0)
        return *reinterpret_cast<const nth_type_of_t<0, Args...> *>(a);
      else
        return {};
    }
  };

  template <typename Data_t, typename Val_t>
  struct EqualHelper
  {
    static constexpr bool test(const v_t &data, const Val_t &val) noexcept
    {
      if constexpr (std::is_same_v<Data_t, Val_t>)
        return val == *std::get<typename std::unique_ptr<Data_t>>(data).get();
      else
        return false;
    }
  };

  void *address;
  v_t data;
};
