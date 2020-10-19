#pragma once

#include <memory>
#include <variant>

#include "data_signature.hpp"
#include "custom_type_utilities.hpp"

template <typename... Types>
class Snapshot
{
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
    return EqualHelper<sizeof...(Types) - 1, Types...>::attempt(data, val);
  }

  template <typename T>
  constexpr bool operator!=(const T &val) const noexcept
  {
    return !(val == *this);
  }

  constexpr void rollback() const noexcept
  {
    RollbackHelper<sizeof...(Types) - 1, Types...>::attempt(address, data);
  }

  constexpr DataSignature get_data_signature() const noexcept
  {
    return DataSignatureHelper<sizeof...(Types) - 1, Types...>::attempt(address, data);
  }

private:
  template <size_t N, typename... Args>
  struct RollbackHelper
  {
    static constexpr void attempt(void *a, const std::variant<typename std::unique_ptr<Args>...> &d) noexcept
    {
      if (d.index() == N)
        *reinterpret_cast<nth_type_of_t<N, Args...> *>(a) = *std::get<N>(d).get();
      else
        RollbackHelper<N - 1, Args...>::attempt(a, d);
    }
  };

  template <typename... Args>
  struct RollbackHelper<0, Args...>
  {
    static constexpr void attempt(void *a, const std::variant<typename std::unique_ptr<Args>...> &d) noexcept
    {
      if (d.index() == 0)
        *reinterpret_cast<nth_type_of_t<0, Args...> *>(a) = *std::get<0>(d).get();
    }
  };

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

  template <size_t N, typename T, typename... Args>
  struct EqualHelper
  {
    static constexpr bool attempt(const std::variant<typename std::unique_ptr<Args>...> &d, const T &val) noexcept
    {
      if (std::is_same_v<T, nth_type_of_t<N, Args...>>)
        return val == *std::get<N>(d).get();
      else
        return EqualHelper<N - 1, Args...>::attempt(d, val);
    }
  };

  template <typename T, typename... Args>
  struct EqualHelper<0, T, Args...>
  {
    static constexpr bool attempt(const std::variant<typename std::unique_ptr<Args>...> &d, const T &val) noexcept
    {
      if (std::is_same_v<T, nth_type_of_t<0, Args...>>)
        return val == *std::get<0>(d).get();
      else
        return false;
    }
  };

  void *address;
  using v_t = std::variant<typename std::unique_ptr<Types>...>;
  v_t data;
};
