#include <unordered_map>
#include <stack>
#include <type_traits>
#include <memory>
#include <variant>

#include <cstdio>

template <typename T, typename... Types>
struct is_type_among
{
  static constexpr bool value = (std::is_same_v<T, Types> || ...);
};

template <typename T, typename... Types>
constexpr bool is_type_among_v = is_type_among<T, Types...>::value;

template <int N, typename... T>
struct nth_type_of;

template <typename T0, typename... Types>
struct nth_type_of<0, T0, Types...>
{
  typedef T0 type;
};
template <int N, typename T0, typename... Types>
struct nth_type_of<N, T0, Types...>
{
  typedef typename nth_type_of<N - 1, Types...>::type type;
};

template <int N, typename... Types>
using nth_type_of_t = typename nth_type_of<N, Types...>::type;

/**
 * @brief Store an element memory address and type
 */
struct DataSignature
{
  const void *address;
  const std::type_info *typeinfo;

  constexpr DataSignature() noexcept
      : address{nullptr},
        typeinfo{nullptr}
  {
  }

  template <typename El_t>
  constexpr DataSignature(const El_t &data) noexcept
      : address{reinterpret_cast<const void *>(&data)},
        typeinfo{&typeid(El_t)}
  {
  }

  constexpr bool operator==(const DataSignature &other) const noexcept
  {
    return address == other.address && typeinfo == other.typeinfo;
  }
};

// specialized hash function for unordered_map keys
struct hash_fn
{
  std::size_t operator()(const DataSignature &ds) const
  {
    std::size_t h1 = std::hash<const void *>()(ds.address);
    std::size_t h2 = std::hash<const std::type_info *>()(ds.typeinfo);

    return h1 ^ h2;
  }
};

template <typename... Types>
class Snapshot
{
public:
  template <typename El_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
  constexpr Snapshot(El_t &element) noexcept
      : address{reinterpret_cast<void *>(&element)},
        data{std::make_unique<El_t>(element)}
  {
  }

  constexpr Snapshot(const Snapshot &other) noexcept
      : address{other.address}
  {
    CopyHelper<sizeof...(Types) - 1, Types...>::attempt(data, other.data);
  }

  template <typename T>
  constexpr bool operator==(const T &val) const noexcept
  {
    return EqualHelper<sizeof...(Types) - 1, Types...>::attempt(data, val);
  }

  template <typename T>
  constexpr bool operator!=(const T &val) const noexcept
  {
    return !(val == *this);
  }

  constexpr void rollback() const noexcept
  {
    RollbackHelper<sizeof...(Types) - 1, Types...>::attempt(address, data);
  }

  constexpr DataSignature get_data_signature() const noexcept
  {
    return DataSignatureHelper<sizeof...(Types) - 1, Types...>::attempt(address, data);
  }

private:
  template <size_t N, typename... Args>
  struct RollbackHelper
  {
    static constexpr void attempt(void *a, const std::variant<typename std::unique_ptr<Args>...> &d) noexcept
    {
      if (d.index() == N)
        *reinterpret_cast<nth_type_of_t<N, Args...> *>(a) = *std::get<N>(d).get();
      else
        RollbackHelper<N - 1, Args...>::attempt(a, d);
    }
  };

  template <typename... Args>
  struct RollbackHelper<0, Args...>
  {
    static constexpr void attempt(void *a, const std::variant<typename std::unique_ptr<Args>...> &d) noexcept
    {
      if (d.index() == 0)
        *reinterpret_cast<nth_type_of_t<0, Args...> *>(a) = *std::get<0>(d).get();
    }
  };

  template <size_t N, typename... Args>
  struct CopyHelper
  {
    static constexpr void attempt(std::variant<typename std::unique_ptr<Args>...> &dest, const std::variant<typename std::unique_ptr<Args>...> &source) noexcept
    {
      if (source.index() == N)
        dest = std::make_unique<nth_type_of_t<N, Args...>>(*std::get<N>(source).get());
      else
        CopyHelper<N - 1, Args...>::attempt(dest, source);
    }
  };

  template <typename... Args>
  struct CopyHelper<0, Args...>
  {
    static constexpr void attempt(std::variant<typename std::unique_ptr<Args>...> &dest, const std::variant<typename std::unique_ptr<Args>...> &source) noexcept
    {
      if (source.index() == 0)
        dest = std::make_unique<nth_type_of_t<0, Args...>>(*std::get<0>(source).get());
    }
  };

  template <size_t N, typename... Args>
  struct DataSignatureHelper
  {
    static constexpr DataSignature attempt(const void *a, const std::variant<typename std::unique_ptr<Args>...> &d) noexcept
    {
      if (d.index() == N)
        return *reinterpret_cast<const nth_type_of_t<N, Args...> *>(a);
      else
        return DataSignatureHelper<N - 1, Args...>::attempt(a, d);
    }
  };

  template <typename... Args>
  struct DataSignatureHelper<0, Args...>
  {
    static constexpr DataSignature attempt(const void *a, const std::variant<typename std::unique_ptr<Args>...> &d) noexcept
    {
      if (d.index() == 0)
        return *reinterpret_cast<const nth_type_of_t<0, Args...> *>(a);
      else
        return {};
    }
  };

  template <size_t N, typename T, typename... Args>
  struct EqualHelper
  {
    static constexpr bool attempt(const std::variant<typename std::unique_ptr<Args>...> &d, const T &val) noexcept
    {
      if (std::is_same_v<T, nth_type_of_t<N, Args...>>)
        return val == *std::get<N>(d).get();
      else
        return EqualHelper<N - 1, Args...>::attempt(d, val);
    }
  };

  template <typename T, typename... Args>
  struct EqualHelper<0, T, Args...>
  {
    static constexpr bool attempt(const std::variant<typename std::unique_ptr<Args>...> &d, const T &val) noexcept
    {
      if (std::is_same_v<T, nth_type_of_t<0, Args...>>)
        return val == *std::get<0>(d).get();
      else
        return false;
    }
  };

  void *address;
  typedef std::variant<typename std::unique_ptr<Types>...> v_t;
  v_t data;
};

template <typename... Types>
class ExternalManager
{
public:
  /**
   * @brief Adds a callback that will be called on every call of the set method
   * @param element Element linked to the callback
   * @param fun Function to be called
   * @return Iterator to the registered callback
   */
  template <typename El_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
  std::unordered_multimap<DataSignature, void *, hash_fn>::iterator register_callback(const El_t &element, void (*fun)(El_t))
  {
    return mCallbackDict.insert({element, reinterpret_cast<void *>(fun)});
  }

  /**
   * @brief Removes all callback associated with element
   * @param element Element associated to the callbacks to be removed
   */
  template <typename El_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
  void remove_callback(const El_t &element)
  {
    mCallbackDict.erase(element);
  }

  /**
   * @brief Removes a callback
   * @param position Callback iterator
   */
  void remove_callback(const std::unordered_multimap<DataSignature, void *, hash_fn>::iterator &position)
  {
    mCallbackDict.erase(position);
  }

  /**
   * @brief Removes all callbacks in the specified range
   * @param first start of the range of callbacks to be removed
   * @param last end of the range of callbacks to be removed (excluded)
   */
  void remove_callback(const std::unordered_multimap<DataSignature, void *, hash_fn>::iterator &first,
                       const std::unordered_multimap<DataSignature, void *, hash_fn>::iterator &last)
  {
    mCallbackDict.erase(first, last);
  }

  template <typename Child_t,
            typename Parent_t,
            typename = std::enable_if_t<is_type_among_v<Child_t, Types...>>,
            typename = std::enable_if_t<is_type_among_v<Parent_t, Types...>>>
  std::unordered_multimap<DataSignature, DataSignature, hash_fn>::iterator register_dependency(const Child_t &child, const Parent_t &parent)
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
  void remove_dependency(const std::unordered_multimap<DataSignature, DataSignature, hash_fn>::iterator &position)
  {
    mDependencies.erase(position);
  }

  /**
   * @brief Removes all dependencies in the specified range
   * @param first start of the range of dependencies to be removed
   * @param last end of the range of dependencies to be removed (excluded)
   */
  void remove_dependency(const std::unordered_multimap<DataSignature, DataSignature, hash_fn>::iterator &first,
                         const std::unordered_multimap<DataSignature, DataSignature, hash_fn>::iterator &last)
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

  template <typename El_t, typename Ret_t, typename... Args_t,
            typename = std::enable_if_t<is_type_among_v<El_t, Types...>>>
  void call(El_t &element, Ret_t El_t::*method(Args_t...), Args_t &&... args)
  {
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
    auto range = mCallbackDict.equal_range(element);
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

  std::unordered_multimap<DataSignature, void *, hash_fn> mCallbackDict;
  std::unordered_multimap<DataSignature, DataSignature, hash_fn> mDependencies; // Child key, Parent mapped
  std::stack<Snapshot<Types...>> mUndos;
  std::stack<Snapshot<Types...>> mRedos;
};

////////////////////////////////////
// EXAMPLE
////////////////////////////////////

#include <cstdio>

struct S
{
  int a = 0;
  int b = 0;

  constexpr int addition_result() const { return a + b; }
};

void isSetS1A(int val)
{
  printf("s1.a was set to %d\n", val);
}

void meh(int val)
{
  printf("meh %d\n", val);
}

void meeeeh(S elem)
{
  printf("result of sum in S: %d\n", elem.addition_result());
}

int main()
{
  S s1;
  ExternalManager<int, S> em;

  auto cb_it = em.register_callback(s1.a, &isSetS1A);
  em.register_dependency(s1.a, s1);
  em.register_dependency(s1.b, s1);

  em.set(s1.a, 10);

  em.set(s1.a, 2);

  em.register_callback(s1, &meeeeh);
  printf("Undo last change --> ");
  em.undo();

  printf("Redo last change --> ");
  em.redo();

  em.set(s1.b, 20);

  em.register_callback(s1.a, &meh);
  em.set(s1.a, 25);

  em.remove_callback(cb_it);

  em.set(s1.a, 30);

  em.remove_callback(s1.a);
  em.remove_dependency(s1.a);
  em.set(s1.a, 2);
  puts("Now printing value of s1.a");
  isSetS1A(s1.a);

  em.set(s1.b, 8);

  return 0;
}
