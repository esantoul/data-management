#pragma once

#include <unordered_map>
#include <stack>

#include "custom_type_utilities.hpp"
#include "snapshot.hpp"
#include "poly_fun.hpp"
#include "signature.hpp"

namespace dmgmt
{
  class StaticDataManager
  {
  private:
    using callback_map_t = std::unordered_multimap<Signature, PolyFun>;
    using dependency_map_t = std::unordered_multimap<Signature, Signature>;

  public:
    using callback_iter_t = callback_map_t::iterator;
    using dependency_iter_t = dependency_map_t::iterator;

    /**
   * @brief Registers a callback that will be called on every element change via StaticDataManager 'set' or 'call' methods calls
   * @param element Element linked to the callback
   * @param fun Function to be called
   * @return Iterator to the registered callback
   */
    template <typename El_t>
    callback_iter_t register_callback(const El_t &element, void (*fun)(El_t))
    {
      return mCallbacks.insert({element, PolyFun::fmt<El_t>(fun)});
    }

    /**
   * @brief Removes all callback associated with element
   * @param element Element associated to the callbacks to be removed
   */
    template <typename El_t>
    void remove_callback(const El_t &element)
    {
      mCallbacks.erase(element);
    }

    /**
   * @brief Removes a callback
   * @param position Callback iterator
   */
    void remove_callback(const callback_iter_t &position)
    {
      mCallbacks.erase(position);
    }

    /**
   * @brief Registers a dependency between two elements. Every child element change via 'set' or 'call' methods will trigger parent element callbacks.
   * @param child Trigger element
   * @param parent Element which callbacks will be triggered subsequently to child change
   * @return Iterator to the registered dependency
   */
    template <typename Child_t, typename Parent_t>
    dependency_iter_t register_dependency(const Child_t &child, const Parent_t &parent)
    {
      return mDependencies.insert({child, parent});
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
   * @param it Dependency iterator
   */
    void remove_dependency(const dependency_iter_t &it)
    {
      mDependencies.erase(it);
    }

    /**
   * @brief Set an element to a given value then calls callbacks & dependencies associated to this element
   * @param element Element to be set
   * @param value New element value
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

      _callback(element);
      _call_dependencies(element);
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

      _callback(element);
      _call_dependencies(element);
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

      _callback(element);
      _call_dependencies(element);

      return result;
    }

    /**
   * @brief Undoes last change, calls all appropriate callbacks & dependencies
   */
    bool undo()
    {
      if (mUndos.size() == 0)
        return false;

      mRedos.push(mUndos.top());
      if (mDirection == Direction::Forward && mUndos.size() > 1)
        mUndos.pop();

      mUndos.top().rollback(snap_callback);
      mUndos.pop();

      mDirection = Direction::Backwards;
      return true;
    }

    /**
   * @brief Redoes last change, calls all appropriate callbacks & dependencies
   */
    bool redo()
    {
      if (mRedos.size() == 0)
        return false;

      mUndos.push(mRedos.top());

      mRedos.top().restore(snap_callback);
      mRedos.pop();

      mDirection = Direction::Forward;
      return true;
    }

  private:
    std::function<void(const Signature &)> snap_callback =
        [&](const Signature &dat) { this->_callback(dat); this->_call_dependencies(dat); };

    void _callback(const Signature &sig) const
    {
      // Call all callbacks directly linked to the element
      auto range = mCallbacks.equal_range(sig);
      for (auto start = range.first; start != range.second; ++start)
        sig.invoke(start->second);
    }

    void _call_dependencies(const Signature &ds) const
    {
      auto parents = mDependencies.equal_range(ds);
      for (auto start = parents.first; start != parents.second; ++start)
        _callback(start->second);
    }

    void clear_redos()
    {
      while (mRedos.size())
        mRedos.pop();
    }

    enum class Direction
    {
      Forward,
      Backwards
    };

    Direction mDirection;

    callback_map_t mCallbacks;
    dependency_map_t mDependencies; // Child key, Parent mapped
    std::stack<SnapshotGroup> mUndos;
    std::stack<SnapshotGroup> mRedos;
  };
} // namespace dmgmt