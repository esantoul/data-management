#pragma once

#include <functional>
#include <typeinfo>

/**
 * @brief Store an element memory address and type
 */
struct DataSignature
{
  const void *address;
  const std::type_info *typeinfo;

  constexpr DataSignature() noexcept
      : address{nullptr},
        typeinfo{nullptr}
  {
  }

  template <typename El_t>
  constexpr DataSignature(const El_t &data) noexcept
      : address{reinterpret_cast<const void *>(&data)},
        typeinfo{&typeid(El_t)}
  {
  }

  constexpr bool operator==(const DataSignature &other) const noexcept
  {
    return address == other.address && typeinfo == other.typeinfo;
  }
};

namespace std
{
  template <>
  class hash<DataSignature>
  {
  public:
    std::size_t operator()(const DataSignature &ds) const
    {
      std::size_t h1 = std::hash<const void *>()(ds.address);
      std::size_t h2 = std::hash<const std::type_info *>()(ds.typeinfo);

      return h1 ^ h2;
    }
  };
} // namespace std
