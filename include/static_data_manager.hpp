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

#include <unordered_map>
#include <unordered_set>
#include <stack>

#include "custom_type_utilities.hpp"
#include "snapshot.hpp"
#include "poly_fun.hpp"
#include "signature.hpp"

namespace dmgmt
{
  /**
   * @brief An object that allows management of static, lifetime controlled data.
   * Allows callbacks & dependencies registration as well as undo/redo management.
   */
  class StaticDataManager
  {
  private:
    using callback_map_t = std::unordered_multimap<Signature, PolyFun>;
    using dependency_map_t = std::unordered_multimap<Signature, Signature>;

  public:
    using callback_iter_t = callback_map_t::iterator;
    using dependency_iter_t = dependency_map_t::iterator;

    /**
     * @brief Registers a callback that will be called on every element change via StaticDataManager set/call methods calls
     * @param element Element linked to the callback
     * @param fun Function to be called
     * @return Iterator to the registered callback
     */
    template <typename El_t, typename Functor_t>
    callback_iter_t register_callback(const El_t &element, const Functor_t &functor)
    {
      return mCallbacks.insert({element, PolyFun::fmt<El_t>(functor)});
    }

    /**
     * @brief Removes all callbacks associated with an element
     * @param element Element associated to the callbacks to be removed
     */
    template <typename El_t>
    void remove_callback(const El_t &element)
    {
      mCallbacks.erase(element);
    }

    /**
     * @brief Removes a callback
     * @param iterator Callback iterator
     */
    void remove_callback(const callback_iter_t &iterator)
    {
      mCallbacks.erase(iterator);
    }

    /**
     * @brief Registers a dependency between two elements.
     * Every source element change via set/call methods will trigger destination element callbacks recursively.
     * @param source Source element
     * @param destination Element which callbacks will be triggered subsequently to child change
     * @return Iterator to the registered dependency
     */
    template <typename Source_t, typename Destination_t>
    dependency_iter_t register_dependency(const Source_t &source, const Destination_t &destination)
    {
      // First check whether the {source, destination} pair already exists
      auto dependencies = mDependencies.equal_range(source);
      for (auto start = dependencies.first; start != dependencies.second; ++start)
        if (start->second == destination)
          return start;
      // If not register this new dependency
      return mDependencies.insert({source, destination});
    }

    /**
     * @brief Removes all dependencies associated with an element
     * @param element Element associated to the dependencies to be removed
     */
    template <typename El_t>
    void remove_dependency(const El_t &element)
    {
      mDependencies.erase(element);
    }

    /**
     * @brief Removes a dependency
     * @param iterator Dependency iterator
     */
    void remove_dependency(const dependency_iter_t &iterator)
    {
      mDependencies.erase(iterator);
    }

    /**
     * @brief Sets an element to a given value then calls callbacks & dependencies associated to this element
     * @param element Element to be set
     * @param value New element value
     * @param groupWithLast set to true if this change needs to be grouped with the previous one in terms of undo/redo
     */
    template <typename El_t>
    void set(El_t &element, const El_t &value, bool groupWithLast = false)
    {
      mDirection = Direction::Forward;
      clear_redos();
      if (!mUndos.size() || mUndos.top() != element)
        mUndos.push(element);

      element = value;

      if (groupWithLast)
        mUndos.top().add(element);
      else
        mUndos.push(element);

      _update(element);
    }

    /**
     * @brief Calls an element non const method then calls callbacks & dependencies associated to this element
     * @param element Element from which the method is called
     * @param method Pointer to one of the element's method
     * @param args Method arguments
     */
    template <typename El_t, typename... Args_t>
    void call(El_t &element, void (El_t::*method)(Args_t...), const Args_t &... args)
    {
      mDirection = Direction::Forward;
      clear_redos();
      if (!mUndos.size() || mUndos.top() != element)
        mUndos.push(element);

      (element.*method)(args...);

      mUndos.push(element);

      _update(element);
    }

    /**
     * @brief Calls an element non const method then calls callbacks & dependencies associated to this element
     * @param element Element from which the method is called
     * @param method Pointer to one of the element's method
     * @param args Method arguments
     * @return Return value of the method
     */
    template <typename El_t, typename Ret_t, typename... Args_t,
              typename = std::enable_if_t<std::is_copy_constructible_v<Ret_t>>>
    Ret_t call(El_t &element, Ret_t (El_t::*method)(Args_t...), const Args_t &... args)
    {
      mDirection = Direction::Forward;
      clear_redos();
      if (!mUndos.size() || mUndos.top() != element)
        mUndos.push(element);

      Ret_t result = (element.*method)(args...);

      mUndos.push(element);

      _update(element);

      return result;
    }

    /**
     * @brief Undoes last change, calls all appropriate callbacks & dependencies
     * @return true if undo was done else false
     */
    bool undo()
    {
      if (mUndos.size() == 0)
        return false;

      mRedos.push(mUndos.top());
      if (mDirection == Direction::Forward && mUndos.size() > 1)
        mUndos.pop();

      mUndos.top().rollback([&](const Signature &ds) { this->_update(ds); });
      mUndos.pop();

      mDirection = Direction::Backwards;
      return true;
    }

    /**
     * @brief Redoes last change, calls all appropriate callbacks & dependencies
     * @return true if redo was done else false
     */
    bool redo()
    {
      if (mRedos.size() == 0)
        return false;

      mUndos.push(mRedos.top());

      mRedos.top().restore([&](const Signature &ds) { this->_update(ds); });
      mRedos.pop();

      mDirection = Direction::Forward;
      return true;
    }

  private:
    /**
     * @brief Calls all callbacks directly linked to the element
     */
    void _callback(const Signature &sig)
    {
      mVisited.insert(sig);
      auto range = mCallbacks.equal_range(sig);
      for (auto start = range.first; start != range.second; ++start)
        sig.invoke(start->second);
    }

    /**
     * @brief Finds all elements that depend of a source element and adds them to the elements to visit
     */
    void _find_next(const Signature &sig)
    {
      mToErase.insert(sig);
      auto dependants = mDependencies.equal_range(sig);
      for (auto start = dependants.first; start != dependants.second; ++start)
        if (mVisited.find(start->second) == mVisited.end())
          mToVisit.insert(start->second);
    }

    /**
     * @brief Calls alls callbacks linked to an element and its dependants recursively.
     * Dependants are reached via a breath first search.
     */
    void _update(const Signature &sig)
    {
      mToVisit.insert(sig);
      while (mToVisit.size()) // Breath first search
      {
        for (const auto &el : mToVisit)
          _callback(el);
        for (const auto &el : mToVisit)
          _find_next(el);
        for (const auto &el : mToErase)
          mToVisit.erase(el);
        mToErase.clear();
      }
      mVisited.clear();
    }

    void clear_redos()
    {
      while (mRedos.size())
        mRedos.pop();
    }

    /**
     * @brief Signals whether last modification was done in direction of the past (Backwards) or the future (Forward).
     */
    enum class Direction
    {
      Forward,
      Backwards
    };

    Direction mDirection;

    callback_map_t mCallbacks;
    dependency_map_t mDependencies; // Source key, destination mapped
    std::stack<SnapshotGroup> mUndos;
    std::stack<SnapshotGroup> mRedos;

    std::unordered_set<Signature> mToVisit;
    std::unordered_set<Signature> mVisited;
    std::unordered_set<Signature> mToErase;
  };
} // namespace dmgmt