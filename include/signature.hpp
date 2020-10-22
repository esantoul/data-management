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

#include "poly_fun.hpp"

namespace dmgmt
{
  class SignatureDataBase
  {
  public:
    virtual ~SignatureDataBase() {}
    virtual const std::type_info &type() const = 0;
    virtual void invoke(const PolyFun &) const = 0;
    virtual const void *address() const = 0;
    virtual SignatureDataBase *clone() const = 0;
  };

  template <typename T>
  class SignatureData : public SignatureDataBase
  {
  public:
    SignatureData(const T &element) : pData{&element} {}
    ~SignatureData() override {}
    const void *address() const override { return pData; }
    const std::type_info &type() const override { return typeid(T); }
    void invoke(const PolyFun &f) const override { f(*pData); }
    SignatureDataBase *clone() const override
    {
      return new SignatureData{*pData};
    }

  private:
    const T *pData;
  };

  class Signature
  {
  public:
    template <typename El_t,
              typename = std::enable_if_t<!std::is_same_v<El_t, Signature>>>
    Signature(const El_t &element)
        : mData{new SignatureData<El_t>{element}}
    {
    }

    Signature(const Signature &other)
        : mData{other.mData->clone()}
    {
    }

    const void *address() const { return mData->address(); }
    const std::type_info &type() const { return mData->type(); }
    void invoke(const PolyFun &f) const { mData->invoke(f); }

    bool operator==(const Signature &other) const
    {
      return mData->type() == other.mData->type() &&
             mData->address() == other.mData->address();
    }

    template <typename El_t,
              typename = std::enable_if_t<!std::is_same_v<El_t, Signature>>>
    bool operator==(const El_t &element)
    {
      return mData->type() == typeid(El_t) &&
             mData->address() == &element;
    }

  private:
    std::unique_ptr<SignatureDataBase> mData;
  };
} // namespace dmgmt

namespace std
{
  template <>
  class hash<dmgmt::Signature>
  {
  public:
    std::size_t operator()(const dmgmt::Signature &s) const
    {
      std::size_t h1 = std::hash<const void *>()(s.address());
      std::size_t h2 = std::hash<const std::type_info *>()(&s.type());

      return h1 ^ h2;
    }
  };
} // namespace std