#pragma once

#include <memory>
#include <typeinfo>

#include "data_signature.hpp"

template <typename T>
class SnapshotData;

class SnapshotDataBase
{
public:
  virtual ~SnapshotDataBase() {}

  virtual bool operator==(const SnapshotDataBase &other) const noexcept = 0;
  bool operator!=(const SnapshotDataBase &other) const noexcept { return !(*this == other); }

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
  virtual DataSignature get_data_sig() const noexcept = 0;
  virtual void rollback() noexcept = 0;

protected:
  template <typename T>
  friend class SnapshotData;
  constexpr SnapshotDataBase(void *value, void *address)
      : value{value},
        address{address}
  {
  }
  void *value;
  void *address;
};

template <typename T>
class SnapshotData : public SnapshotDataBase
{
public:
  constexpr SnapshotData(T &element)
      : SnapshotDataBase{new T(element), &element}
  {
  }

  ~SnapshotData()
  {
    if (!std::is_trivially_constructible<T>::value)
      delete static_cast<T *>(value);
  }

  bool operator==(const SnapshotDataBase &other) const noexcept override
  {
    return typeid(T) == *other.get_typeinfo() &&
           address == other.address &&
           *static_cast<T *>(value) == *static_cast<T *>(other.value);
  }

  const std::type_info *get_typeinfo() const noexcept override { return &typeid(T); }

  DataSignature get_data_sig() const noexcept override { return *static_cast<T *>(address); }

  void rollback() noexcept override { *static_cast<T *>(address) = *static_cast<T *>(value); }
};

class Snapshot
{
public:
  template <typename El_t>
  Snapshot(El_t &element)
      : mData{new SnapshotData<El_t>(element)}
  {
  }

  Snapshot(Snapshot &&other)
      : mData{std::move(other.mData)}
  {
  }

  bool operator==(const Snapshot &other) { return *mData.get() == *other.mData.get(); }

  template <typename Val_t>
  bool operator==(const Val_t &value) { return *mData.get() == value; }

  void rollback() { mData->rollback(); }

  DataSignature get_data_sig() { return mData->get_data_sig(); }

private:
  std::unique_ptr<SnapshotDataBase> mData;
};