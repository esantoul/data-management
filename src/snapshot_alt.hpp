#pragma once

#include <memory>
#include <typeinfo>

#include "data_signature.hpp"

template <typename T>
class Snapshot_Alt;

class Snapshot_Alt_Base
{
public:
  virtual ~Snapshot_Alt_Base() {}

  virtual bool operator==(const Snapshot_Alt_Base &other) const noexcept = 0;
  bool operator!=(const Snapshot_Alt_Base &other) const noexcept { return !(*this == other); }

  template <typename Val_t>
  bool operator==(const Val_t &val) const noexcept
  {
    return typeid(Val_t) == *get_typeinfo() &&
           &val == address &&
           val == *static_cast<Val_t *>(value);
  }

  template <typename Val_t>
  bool operator!=(const Val_t &val) const noexcept { return !(*this == val); }

  virtual const std::type_info *get_typeinfo() const noexcept = 0;
  virtual DataSignature get_data_signature() const noexcept = 0;
  virtual void rollback() noexcept = 0;

protected:
  template <typename T>
  friend class Snapshot_Alt;
  Snapshot_Alt_Base(void *value, void *address)
      : value{value},
        address{address}
  {
  }
  void *value;
  void *address;
};

template <typename T>
class Snapshot_Alt : public Snapshot_Alt_Base
{
public:
  Snapshot_Alt(T &element)
      : Snapshot_Alt_Base{new T(element), &element}
  {
  }

  ~Snapshot_Alt()
  {
    if (!std::is_trivially_constructible<T>::value)
      delete static_cast<T *>(value);
  }

  bool operator==(const Snapshot_Alt_Base &other) const noexcept override
  {
    return typeid(T) == *other.get_typeinfo() &&
           address == other.address &&
           *static_cast<T *>(value) == *static_cast<T *>(other.value);
  }

  const std::type_info *get_typeinfo() const noexcept override { return &typeid(T); }

  DataSignature get_data_signature() const noexcept override { return *static_cast<T *>(address); }

  void rollback() noexcept override { *static_cast<T *>(address) = *static_cast<T *>(value); }
};

template <typename T>
Snapshot_Alt<T> make_snapshot(T &value)
{
  return {value};
}

template <typename T>
std::unique_ptr<Snapshot_Alt<T>> make_snapshot_uptr(T &value)
{
  return std::unique_ptr<Snapshot_Alt<T>>{new Snapshot_Alt<T>(value)};
}
