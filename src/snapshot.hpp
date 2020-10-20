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
    return data.index() == get_type_index_v<Val_t, Types...> &&           // Check that data currently holds a std::unique_ptr to Val_t
           static_cast<const void *>(&val) == address &&                  // Check that address and val address are the same
           *std::get<typename std::unique_ptr<Val_t>>(data).get() == val; // Check that data holds the same value as val
  }

  template <typename Val_t,
            typename = std::enable_if_t<is_type_among_v<Val_t, Types...>>>
  constexpr bool operator!=(const Val_t &val) const noexcept
  {
    return !(*this == val);
  }

  constexpr void rollback() const noexcept
  {
    void(((data.index() == get_type_index_v<Types, Types...> && // void operator to avoid unused variable warning
           (bool)&(*reinterpret_cast<Types *>(address) = *std::get<typename std::unique_ptr<Types>>(data).get())) ||
          ...)); // using fold expression & binary operators short-circuiting
  }

  constexpr DataSignature get_data_sig() const noexcept
  {
    DataSignature ds{};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
    void(((data.index() == get_type_index_v<Types, Types...> &&
           (bool)&(ds = *reinterpret_cast<Types *>(address))) ||
          ...));
#pragma GCC diagnostic pop
    return ds;
  }

private:
  void *address;
  v_t data;
};
