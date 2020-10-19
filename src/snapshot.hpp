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
    void(((other.data.index() == get_type_index_v<Types, Types...> &&
           (bool)&(data = std::make_unique<Types>(*std::get<typename std::unique_ptr<Types>>(other.data).get()))) ||
          ...));
  }

  constexpr bool operator==(const Snapshot &other) const noexcept
  {
    return other.address == address &&
           other.data.index() == data.index() &&
           ((data.index() == get_type_index_v<Types, Types...> &&
             *std::get<typename std::unique_ptr<Types>>(other.data).get() == *std::get<typename std::unique_ptr<Types>>(data).get()) ||
            ...);
  }

  constexpr bool operator!=(const Snapshot &other) const noexcept
  {
    return !(*this == other);
  }

  template <typename Val_t,
            typename = std::enable_if_t<is_type_among_v<Val_t, Types...>>>
  constexpr bool operator==(const Val_t &val) const noexcept
  {
    // Check whether data currently holds a std::unique_ptr to Val_t
    if (data.index() != get_type_index_v<Val_t, Types...>)
      return false;
    // Check whether address and val address are the same
    if (static_cast<const void *>(&val) != address)
      return false;
    // Check whether data holds the same value as val
    return ((std::is_same_v<Val_t, Types> && EqualHelper<Types, Val_t>::test(data, val)) || ...);
  }

  template <typename Val_t,
            typename = std::enable_if_t<is_type_among_v<Val_t, Types...>>>
  constexpr bool operator!=(const Val_t &val) const noexcept
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
