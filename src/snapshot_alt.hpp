#pragma once

#include <memory>
#include <typeinfo>
#include <vector>

#include "data_signature.hpp"

template <typename T>
class SnapshotData;

class SnapshotDataBase
{
public:
  virtual ~SnapshotDataBase() {}

  virtual bool operator==(const SnapshotDataBase &other) const = 0;
  bool operator!=(const SnapshotDataBase &other) const { return !(*this == other); }

  template <typename Val_t>
  bool operator==(const Val_t &val) const
  {
    return typeid(Val_t) == *get_typeinfo() &&
           &val == address &&
           val == *static_cast<Val_t *>(value);
  }

  template <typename Val_t>
  bool operator!=(const Val_t &val) const { return !(*this == val); }

  virtual const std::type_info *get_typeinfo() const = 0;
  virtual DataSignature get_data_sig() const = 0;
  virtual void rollback() = 0;

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

  bool operator==(const SnapshotDataBase &other) const override
  {
    return typeid(T) == *other.get_typeinfo() &&
           address == other.address &&
           *static_cast<T *>(value) == *static_cast<T *>(other.value);
  }

  const std::type_info *get_typeinfo() const override { return &typeid(T); }

  DataSignature get_data_sig() const override { return *static_cast<T *>(address); }

  void rollback() override { *static_cast<T *>(address) = *static_cast<T *>(value); }
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

  bool operator==(const Snapshot &other) const { return *mData.get() == *other.mData.get(); }

  template <typename Val_t>
  bool operator==(const Val_t &value) const { return *mData.get() == value; }

  void rollback() { mData->rollback(); }

  DataSignature get_data_sig() const { return mData->get_data_sig(); }

private:
  std::unique_ptr<SnapshotDataBase> mData;
};

class SnapshotGroup
{
public:
  SnapshotGroup() {}

  SnapshotGroup(SnapshotGroup &&other)
      : mData{std::move(other.mData)}
  {
  }

  const Snapshot *get_last() { return mData.size() ? &mData.at(mData.size() - 1) : nullptr; }

  template <typename El_t>
  void add(El_t &element) { mData.push_back(element); }

  void rollback(std::function<void(const DataSignature &)> f = nullptr)
  {
    for (auto it = mData.rbegin(); it != mData.rend(); ++it)
    {
      it->rollback();
      if (f)
        f(it->get_data_sig());
    }
  }

  void restore(std::function<void(const DataSignature &)> f = nullptr)
  {
    for (auto it = mData.begin(); it != mData.end(); ++it)
    {
      it->rollback();
      if (f)
        f(it->get_data_sig());
    }
  }

private:
  std::vector<Snapshot> mData;
};

/*

#include <cstdio>

struct Hello
{
  Hello() { puts("Hello"); }
  Hello(const Hello &h)
  {
    (void)h;
    puts("Hello cpy");
  }
  const Hello &operator=(const Hello &h)
  {
    (void)h;
    puts("Hello assign");
    return *this;
  }
  ~Hello() { puts("Goodbye"); }
  bool operator==(const Hello &other) const { return &other == this; }
};

struct Consumer
{
  void consume(const DataSignature &dat)
  {
    printf("[%s]\n", dat.typeinfo->name());
  }
};

int main()
{
  int i = 1;
  int j = 10;
  float f = 10.f;
  Hello h;
  SnapshotGroup grp;
  grp.add(i);
  grp.add(j);
  grp.add(f);
  grp.add(h);
  Consumer c;
  i = j = f = 0;
  SnapshotGroup grp2{std::move(grp)};
  grp2.rollback([&](const DataSignature &dat) { c.consume(dat); });
  return i + j + f;
}

*/