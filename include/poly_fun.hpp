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

#include <functional>
#include <memory>
#include <cassert>

namespace dmgmt
{
  class PolyFunDataBase
  {
  public:
    virtual ~PolyFunDataBase() {}
    virtual void operator()(const void *) const = 0;
    virtual const std::type_info &type() const = 0;
    virtual PolyFunDataBase *clone() const = 0;

  protected:
  };

  template <typename El_t>
  class PolyFunData : public PolyFunDataBase
  {
  public:
    PolyFunData(const std::function<void(const El_t &)> &fun)
        : mFun(fun)
    {
    }

    ~PolyFunData() {}

    void operator()(const void *data_ptr) const override
    {
      mFun(*static_cast<const El_t *>(data_ptr));
    }

    const std::type_info &type() const override { return typeid(El_t); }

    PolyFunDataBase *clone() const override { return new PolyFunData{mFun}; }

  private:
    std::function<void(const El_t &)> mFun;
  };

  /**
   * @brief An object that contains a polymorphic callable.
   * Can contain any callable with signature void(El_t) where El_t is any type.
   */
  class PolyFun
  {
  public:
    template <typename El_t,
              typename = std::enable_if_t<!std::is_same_v<El_t, PolyFun>>>
    PolyFun(const std::function<void(const El_t &)> &fun)
        : mFunData{new PolyFunData<El_t>(fun)}
    {
    }

    PolyFun(const PolyFun &other)
        : mFunData{other.mFunData->clone()}
    {
    }

    template <typename El_t>
    void operator()(const El_t &data) const
    {
      if (typeid(El_t) == mFunData->type())
        (*mFunData.get())(&data);
      else
        assert(false);
    }

    /**
     * @brief Formats any functor to a type handlable by PolyFun
     * @tparam Dat_t Type of the argument handled by the functor
     * @param functor a functor with void(const Dat_t&) signature
     * @return Functor properly converted to type std::function<void(const Dat_t&)>
     */
    template <typename Dat_t, typename Functor_t>
    static std::function<void(const Dat_t &)> fmt(const Functor_t &functor)
    {
      return functor;
    }

  private:
    std::unique_ptr<PolyFunDataBase> mFunData;
  };
} // namespace dmgmt
