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

#include "static_data_manager.hpp"
#include <cassert>

namespace dmgmt
{
  template <typename Data_t>
  class DataManager
  {
  public:
    const Data_t &get() { return mData; }

    template <typename Child_t, typename Parent_t>
    StaticDataManager::dependency_iter_t register_dependency(const Child_t &child, const Parent_t &parent)
    {
      assert("child cannot be accessed by DataManager!!" && isValidMemory(child));
      assert("parent cannot be accessed by DataManager!!" && isValidMemory(parent));
      return mManager.register_dependency(child, parent);
    }

    template <typename Child_t>
    void remove_dependency(const Child_t &child)
    {
      mManager.remove_dependency(child);
    }

    void remove_dependency(StaticDataManager::dependency_iter_t iterator)
    {
      mManager.remove_dependency(iterator);
    }

    template <typename El_t>
    StaticDataManager::callback_iter_t register_callback(const El_t &element, void (*callback)(El_t))
    {
      assert("element cannot be accessed by DataManager!!" && isValidMemory(element));
      return mManager.register_callback(element, callback);
    }

    template <typename El_t>
    void remove_callback(const El_t &element)
    {
      mManager.remove_callback(element);
    }

    void remove_callback(StaticDataManager::callback_iter_t iterator)
    {
      mManager.remove_callback(iterator);
    }

    template <typename El_t>
    void set(const El_t &element, const El_t &value, bool groupWithLast = false)
    {
      assert("element cannot be accessed by DataManager!!" && isValidMemory(element));
      mManager.set(const_cast<El_t &>(element), value, groupWithLast);
    }

    template <typename El_t, typename Ret_t, typename... Args_t>
    Ret_t call(const El_t &element, Ret_t (El_t::*method)(Args_t...), const Args_t &... args)
    {
      assert("element cannot be accessed by DataManager!!" && isValidMemory(element));
      return mManager.call(const_cast<El_t &>(element), method, args...);
    }

    template <typename El_t, typename... Args_t>
    void call(const El_t &element, void (El_t::*method)(Args_t...), const Args_t &... args)
    {
      assert("element cannot be accessed by DataManager!!" && isValidMemory(element));
      mManager.call(const_cast<El_t &>(element), method, args...);
    }

    void undo() { mManager.undo(); }
    void redo() { mManager.redo(); }

  private:
    template <typename El_t>
    bool isValidMemory(const El_t &element)
    {
      return size_t(&element) >= size_t(&mData) && std::size_t(&element + 1) <= std::size_t(&mData + 1);
    }

    Data_t mData;
    StaticDataManager mManager;
  };
} // namespace dmgmt