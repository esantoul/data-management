#pragma once

#include <memory>
#include <typeinfo>
#include <functional>
#include <vector>

#include "custom_type_utilities.hpp"
#include "signature.hpp"

namespace dmgmt
{
  class SnapshotDataBase
  {
  public:
    virtual ~SnapshotDataBase(){};

    virtual SnapshotDataBase *clone() const = 0;

    bool operator==(const SnapshotDataBase &other) const
    {
      return type() == other.type() &&
             address() == other.address() &&
             has_same_data(other.data());
    }

    template <typename El_t>
    bool operator==(const El_t &element) const
    {
      return typeid(El_t) == type() &&
             &element == address() &&
             has_same_data(&element);
    }

    template <typename El_t>
    bool holds(const El_t &element) const
    {
      return typeid(El_t) == type() &&
             &element == address();
    }

    virtual void rollback(std::function<void(const Signature &)> callback = nullptr) = 0;

  protected:
    virtual const std::type_info &type() const = 0;
    virtual const void *address() const = 0;
    virtual const void *data() const = 0;
    virtual bool has_same_data(const void *) const = 0;
  };

  template <typename T>
  class SnapshotData : public SnapshotDataBase
  {
  public:
    SnapshotData(T &element)
        : mData{element},
          pAddress{&element}
    {
    }

    ~SnapshotData() override {}

    SnapshotDataBase *clone() const override { return new SnapshotData(mData, pAddress); }

    void rollback(std::function<void(const Signature &)> callback = nullptr) override
    {
      *pAddress = mData;
      if (callback)
        callback({*pAddress});
    }

  private:
    SnapshotData(const T &value, T *address)
        : mData{value},
          pAddress{address}
    {
    }

    bool has_same_data(const void *data_ptr) const override
    {
      if constexpr (has_operator_equal_v<T>)
        return mData == *static_cast<const T *>(data_ptr);
      else
      {
        (void)data_ptr;
        return false;
      }
    }

    const void *data() const override { return &mData; }

    const std::type_info &type() const override { return typeid(T); }
    const void *address() const override { return pAddress; }

    T mData;
    T *pAddress;
  };

  class Snapshot
  {
  public:
    template <typename El_t,
              typename = std::enable_if_t<!std::is_same_v<El_t, Snapshot>>>
    Snapshot(El_t &element)
        : mData{new SnapshotData<El_t>{element}}
    {
    }

    Snapshot(const Snapshot &other)
        : mData{other.mData->clone()}
    {
    }

    Snapshot(Snapshot &&other)
        : mData{std::move(other.mData)}
    {
    }

    bool valid() const { return bool(mData); }

    template <typename T>
    bool operator==(const T &other) const
    {
      if (!mData)
        return false;
      return *mData.get() == other;
    }

    template <typename T>
    bool operator!=(const T &other) const
    {
      return !(*this == other);
    }

    bool operator==(const Snapshot &other) const
    {
      if (!mData)
        return false;
      return *mData.get() == *other.mData.get();
    }

    bool operator!=(const Snapshot &other) const
    {
      return !(*this == other);
    }

    template <typename El_t>
    bool holds(const El_t &element) const
    {
      if (!mData)
        return false;
      return mData.get()->holds(element);
    }

    void rollback(std::function<void(const Signature &)> callback = nullptr)
    {
      if (!mData)
        return;
      mData.get()->rollback(callback);
    }

  private:
    std::unique_ptr<SnapshotDataBase> mData;
  };

  class SnapshotGroup
  {
  public:
    SnapshotGroup() = default;

    template <typename... Els_t>
    SnapshotGroup(Els_t &... elements)
        : mSnapshots{elements...}
    {
    }

    template <typename El_t>
    bool operator==(const El_t &element) const { return last() && *last() == element; }

    template <typename El_t>
    bool operator!=(const El_t &element) const { return !last() || *last() != element; }

    std::size_t size() const { return mSnapshots.size(); }

    template <typename El_t>
    void add(El_t &element) { mSnapshots.push_back(element); }

    const Snapshot *last() const
    {
      if (mSnapshots.empty())
        return nullptr;
      return &*(mSnapshots.cend() - 1);
    }

    void rollback(std::function<void(const Signature &)> callback = nullptr)
    {
      for (auto start = mSnapshots.rbegin(); start != mSnapshots.rend(); ++start)
        start->rollback(callback);
    }

    void restore(std::function<void(const Signature &)> callback = nullptr)
    {
      for (auto start = mSnapshots.begin(); start != mSnapshots.end(); ++start)
        start->rollback(callback);
    }

  private:
    std::vector<Snapshot> mSnapshots;
  };
} // namespace dmgmt