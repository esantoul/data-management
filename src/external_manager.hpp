#include <unordered_map>
#include <stack>

#include "custom_type_utilities.hpp"
#include "data_signature.hpp"
#include "snapshot.hpp"

template <typename... Types>
class ExternalManager
{
private:
  using callback_map_t = std::unordered_multimap<DataSignature, void *, DataSignature::hash_fn>;
  using dependency_map_t = std::unordered_multimap<DataSignature, DataSignature, DataSignature::hash_fn>;

public:
  using callback_iter_t = callback_map_t::iterator;
  using dependency_iter_t = dependency_map_t::iterator;

  /**
   * @brief Adds a callback that will be called on every call of the set method
   * @param element Element linked to the callback
   * @param fun Function to be called
   * @return Iterator to the registered callback
   */
  template <typename El_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
  callback_iter_t register_callback(const El_t &element, void (*fun)(El_t))
  {
    return mCallbacks.insert({element, reinterpret_cast<void *>(fun)});
  }

  /**
   * @brief Removes all callback associated with element
   * @param element Element associated to the callbacks to be removed
   */
  template <typename El_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
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
   * @brief Removes all callbacks in the specified range
   * @param first start of the range of callbacks to be removed
   * @param last end of the range of callbacks to be removed (excluded)
   */
  void remove_callback(const callback_iter_t &first,
                       const callback_iter_t &last)
  {
    mCallbacks.erase(first, last);
  }

  template <typename Child_t,
            typename Parent_t,
            typename = std::enable_if_t<is_type_among_v<Child_t, Types...>>,
            typename = std::enable_if_t<is_type_among_v<Parent_t, Types...>>>
  dependency_iter_t register_dependency(const Child_t &child, const Parent_t &parent)
  {
    return mDependencies.insert({child, parent});
  }

  /**
   * @brief Removes all dependencies associated with element
   * @param element Element associated to the dependencies to be removed
   */
  template <typename El_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
  void remove_dependency(const El_t &element)
  {
    mDependencies.erase(element);
  }

  /**
   * @brief Removes a dependency
   * @param position Dependency iterator
   */
  void remove_dependency(const dependency_iter_t &position)
  {
    mDependencies.erase(position);
  }

  /**
   * @brief Removes all dependencies in the specified range
   * @param first start of the range of dependencies to be removed
   * @param last end of the range of dependencies to be removed (excluded)
   */
  void remove_dependency(const dependency_iter_t &first,
                         const dependency_iter_t &last)
  {
    mDependencies.erase(first, last);
  }

  /**
   * @brief Set a variable to a given value and calls callbacks and dependencies associated to this value
   * @param element Reference to the element to be set
   * @param value Value to set the element to
   */
  template <typename El_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
  void set(El_t &element, const El_t &value)
  {
    mDirection = Direction::Forward;
    if (!mUndos.size() || mUndos.top() != DataSignature{element})
      mUndos.push(element);

    element = value;

    mUndos.push(element);

    _callback(element);
    _call_dependencies(element);
  }

  template <typename El_t, typename... Args_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
  void call(El_t &element, void (El_t::*method)(Args_t...), const Args_t &... args)
  {
    (element.*method)(args...);
    _callback(element);
    _call_dependencies(element);
  }

  template <typename El_t, typename Ret_t, typename... Args_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
  Ret_t call(El_t &element, Ret_t (El_t::*method)(Args_t...), const Args_t &... args)
  {
    Ret_t result = (element.*method)(args...);
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
    auto data_sig = mUndos.top().get_data_signature();
    mUndos.top().rollback();
    mUndos.pop();

    _callback(data_sig);
    _call_dependencies(data_sig);

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
    auto data_sig = mRedos.top().get_data_signature();
    mRedos.top().rollback();
    mRedos.pop();

    _callback(data_sig);
    _call_dependencies(data_sig);

    mDirection = Direction::Forward;
    return true;
  }

private:
  template <typename El_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
  void _callback(const El_t &element) const
  {
    // Call all callbacks directly linked to the element
    auto range = mCallbacks.equal_range(element);
    for (auto start = range.first; start != range.second; ++start)
      (*reinterpret_cast<void (*)(El_t)>((*start).second))(element);
  }

  void _callback(const DataSignature &ds) const
  {
    (_try_callback<Types>(ds), ...);
  }

  template <typename T>
  void _try_callback(const DataSignature &ds) const
  {
    if (typeid(T) == *ds.typeinfo)
      _callback(*reinterpret_cast<const T *>(ds.address));
  }

  void _call_dependencies(const DataSignature &ds) const
  {
    auto parents = mDependencies.equal_range(ds);
    for (auto start = parents.first; start != parents.second; ++start)
      _callback(start->second);
  }

  enum class Direction
  {
    Forward,
    Backwards
  };

  Direction mDirection;

  callback_map_t mCallbacks;
  dependency_map_t mDependencies; // Child key, Parent mapped
  std::stack<Snapshot<Types...>> mUndos;
  std::stack<Snapshot<Types...>> mRedos;
};
