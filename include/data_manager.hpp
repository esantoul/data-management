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
  /**
   * @brief An object containing a class/struct, that allows callback and dependency registration as well as undo/redo management
   * @tparam Data_t Type of the contained & manageable data
   */
  template <typename Data_t>
  class DataManager
  {
  public:
    /**
     * @brief Returns a const reference to the data stored in the manager.
     * This is to be used for set & call methods first argument.
     */
    const Data_t &get() { return mData; }

    /**
     * @brief Registers a callback that will be called on every element change via DataManager set/call methods calls
     * @param element Element linked to the callback
     * @param fun Function to be called
     * @return Iterator to the registered callback
     */
    template <typename El_t, typename Functor_t>
    StaticDataManager::callback_iter_t register_callback(const El_t &element, const Functor_t &functor)
    {
      assert("element cannot be accessed by DataManager!!" && isValidMemory(element));
      return mManager.register_callback(element, functor);
    }

    /**
     * @brief Removes all callback associated with an element
     * @param element Element associated to the callbacks to be removed
     */
    template <typename El_t>
    void remove_callback(const El_t &element)
    {
      mManager.remove_callback(element);
    }

    /**
     * @brief Removes a callback
     * @param iterator Callback iterator
     */
    void remove_callback(StaticDataManager::callback_iter_t iterator)
    {
      mManager.remove_callback(iterator);
    }

    /**
     * @brief Registers a dependency between two elements.
     * Every child element change via 'set' or 'call' methods will trigger parent element callbacks recursively.
     * @param child Trigger element
     * @param parent Element which callbacks will be triggered subsequently to child change
     * @return Iterator to the registered dependency
     */
    template <typename Child_t, typename Parent_t>
    StaticDataManager::dependency_iter_t register_dependency(const Child_t &child, const Parent_t &parent)
    {
      assert("child cannot be accessed by DataManager!!" && isValidMemory(child));
      assert("parent cannot be accessed by DataManager!!" && isValidMemory(parent));
      return mManager.register_dependency(child, parent);
    }

    /**
     * @brief Removes all dependencies associated with an element
     * @param element Element associated to the dependencies to be removed
     */
    template <typename El_t>
    void remove_dependency(const El_t &element)
    {
      mManager.remove_dependency(element);
    }

    /**
     * @brief Removes a dependency
     * @param iterator Dependency iterator
     */
    void remove_dependency(StaticDataManager::dependency_iter_t iterator)
    {
      mManager.remove_dependency(iterator);
    }

    /**
     * @brief Sets an element to a given value then calls callbacks & dependencies associated to this element
     * @param element Element to be set
     * @param value New element value
     * @param groupWithLast set to true if this change needs to be grouped with the previous one in terms of undo/redo
     */
    template <typename El_t>
    void set(const El_t &element, const El_t &value, bool groupWithLast = false)
    {
      assert("element cannot be accessed by DataManager!!" && isValidMemory(element));
      mManager.set(const_cast<El_t &>(element), value, groupWithLast);
    }

    /**
     * @brief Calls an element non const method then calls callbacks & dependencies associated to this element
     * @param element Element from which the method is called
     * @param method Pointer to one of the element's method
     * @param args Method arguments
     * @return Return value of the method
     */
    template <typename El_t, typename Ret_t, typename... Args_t>
    Ret_t call(const El_t &element, Ret_t (El_t::*method)(Args_t...), const Args_t &... args)
    {
      assert("element cannot be accessed by DataManager!!" && isValidMemory(element));
      return mManager.call(const_cast<El_t &>(element), method, args...);
    }

    /**
     * @brief Calls an element non const method then calls callbacks & dependencies associated to this element
     * @param element Element from which the method is called
     * @param method Pointer to one of the element's method
     * @param args Method arguments
     */
    template <typename El_t, typename... Args_t>
    void call(const El_t &element, void (El_t::*method)(Args_t...), const Args_t &... args)
    {
      assert("element cannot be accessed by DataManager!!" && isValidMemory(element));
      mManager.call(const_cast<El_t &>(element), method, args...);
    }

    /**
     * @brief Undoes last change, calls all appropriate callbacks & dependencies
     * @return true if undo was done else false
     */
    bool undo() { return mManager.undo(); }

    /**
     * @brief Redoes last change, calls all appropriate callbacks & dependencies
     * @return true if redo was done else false
     */
    bool redo() { return mManager.redo(); }

  private:
    /**
     * @brief Check whether an element belongs to the stored data structure
     */
    template <typename El_t>
    bool isValidMemory(const El_t &element)
    {
      return size_t(&element) >= size_t(&mData) && std::size_t(&element + 1) <= std::size_t(&mData + 1);
    }

    Data_t mData;
    StaticDataManager mManager;
  };
} // namespace dmgmt