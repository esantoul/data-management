#include <unordered_map>
#include <stack>
#include <type_traits>
#include <memory>
#include <variant>

struct DataSignature
{
  const void *address;
  const std::type_info *typeinfo;

  DataSignature()
      : address{nullptr},
        typeinfo{nullptr}
  {
  }

  template <typename El_t>
  DataSignature(const El_t &data)
      : address{reinterpret_cast<const void *>(&data)},
        typeinfo{&typeid(El_t)}
  {
  }

  bool operator==(const DataSignature &other) const
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
  template <typename El_t>
  Snapshot(El_t &element)
      : address{reinterpret_cast<void *>(&element)},
        data{std::make_unique<El_t>(element)}
  {
  }

  template <typename T>
  bool operator==(const T &val)
  {
    return EqualHelper<sizeof...(Types) - 1, Types...>::test(data, val);
  }

  template <typename T>
  bool operator!=(const T &val)
  {
    return !(val == *this);
  }

  Snapshot(const Snapshot &other)
      : address{other.address}
  {
    CopyHelper<sizeof...(Types) - 1, Types...>::test(data, other.data);
  }

  void rollback()
  {
    RollbackHelper<sizeof...(Types) - 1, Types...>::test(address, data);
  }

  DataSignature get_data_signature()
  {
    return DataSignatureHelper<sizeof...(Types) - 1, Types...>::test(address, data);
  }

private:
  template <int N, typename... T>
  struct tuple_element;

  template <typename T0, typename... T>
  struct tuple_element<0, T0, T...>
  {
    typedef T0 type;
  };
  template <int N, typename T0, typename... T>
  struct tuple_element<N, T0, T...>
  {
    typedef typename tuple_element<N - 1, T...>::type type;
  };

  template <size_t N, typename... Args>
  struct RollbackHelper
  {
    static void test(void *a, const std::variant<typename std::unique_ptr<Args>...> &d)
    {
      if (d.index() == N)
        *reinterpret_cast<typename tuple_element<N, Args...>::type *>(a) = *std::get<N>(d).get();
      else
        RollbackHelper<N - 1, Args...>::test(a, d);
    }
  };

  template <typename... Args>
  struct RollbackHelper<0, Args...>
  {
    static void test(void *a, const std::variant<typename std::unique_ptr<Args>...> &d)
    {
      if (d.index() == 0)
        *reinterpret_cast<typename tuple_element<0, Args...>::type *>(a) = *std::get<0>(d).get();
    }
  };

  template <size_t N, typename... Args>
  struct CopyHelper
  {
    static void test(std::variant<typename std::unique_ptr<Args>...> &dest, const std::variant<typename std::unique_ptr<Args>...> &source)
    {
      if (source.index() == N)
        dest = std::make_unique<typename tuple_element<N, Args...>::type>(*std::get<0>(source).get());
      else
        CopyHelper<N - 1, Args...>::test(dest, source);
    }
  };

  template <typename... Args>
  struct CopyHelper<0, Args...>
  {
    static void test(std::variant<typename std::unique_ptr<Args>...> &dest, const std::variant<typename std::unique_ptr<Args>...> &source)
    {
      if (source.index() == 0)
        dest = std::make_unique<typename tuple_element<0, Args...>::type>(*std::get<0>(source).get());
    }
  };

  template <size_t N, typename... Args>
  struct DataSignatureHelper
  {
    static DataSignature test(const void *a, const std::variant<typename std::unique_ptr<Args>...> &d)
    {
      if (d.index() == N)
        return *reinterpret_cast<const typename tuple_element<N, Args...>::type *>(a);
      else
        return DataSignatureHelper<N - 1, Args...>::test(a, d);
    }
  };

  template <typename... Args>
  struct DataSignatureHelper<0, Args...>
  {
    static DataSignature test(const void *a, const std::variant<typename std::unique_ptr<Args>...> &d)
    {
      if (d.index() == 0)
        return *reinterpret_cast<const typename tuple_element<0, Args...>::type *>(a);
      else
        return {};
    }
  };

  template <size_t N, typename T, typename... Args>
  struct EqualHelper
  {
    static bool test(const std::variant<typename std::unique_ptr<Args>...> &d, const T &val)
    {
      if (std::is_same_v<T, tuple_element<N, Args...>::type>)
        return val == *std::get<N>(d).get();
      else
        return EqualHelper<N - 1, Args...>::test(d, val);
    }
  };

  template <typename T, typename... Args>
  struct EqualHelper<0, T, Args...>
  {
    static bool test(const std::variant<typename std::unique_ptr<Args>...> &d, const T &val)
    {
      if (std::is_same_v<T, tuple_element<0, Args...>::type>)
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
  template <typename El_t>
  void removeCallback(const El_t &element)
  {
    mCallbackDict.erase(element);
  }

  template <typename El_t, typename Arg_t>
  void registerCallback(const El_t &element, void (*fun)(Arg_t))
  {
    removeCallback(element);
    mCallbackDict.insert({element, reinterpret_cast<void *>(fun)});
  }

  template <typename El_t>
  void set(El_t &element, const El_t &value)
  {
    mDirection = Direction::Forward;
    if (!mUndos.size() || mUndos.top() != DataSignature{element})
      mUndos.push(element);

    element = value;

    mUndos.push(element);

    callbackAttachedTo(element);
  }

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

    callbackFromKey(data_sig);

    mDirection = Direction::Backwards;
    return true;
  }

  bool redo()
  {
    if (mRedos.size() == 0)
      return false;

    mUndos.push(mRedos.top());
    auto data_sig = mRedos.top().get_data_signature();
    mRedos.top().rollback();
    mRedos.pop();

    callbackFromKey(data_sig);

    mDirection = Direction::Forward;
    return true;
  }

private:
  void callbackFromKey(const DataSignature &ds)
  {
    (tryCallbackWithType<Types>(ds), ...);
  }

  template <typename T>
  void tryCallbackWithType(const DataSignature &ds)
  {
    if (typeid(T) == *ds.typeinfo)
      callbackAttachedTo(*reinterpret_cast<const T *>(ds.address));
  }

  template <typename El_t>
  void callbackAttachedTo(const El_t &element)
  {
    auto f = mCallbackDict.find(element);
    if (f != mCallbackDict.end())
      (*reinterpret_cast<void (*)(El_t)>((*f).second))(element);
  }

  enum class Direction
  {
    Forward,
    Backwards
  };

  Direction mDirection;

  std::unordered_multimap<DataSignature, void *, hash_fn> mCallbackDict;
  std::stack<Snapshot<Types...>> mUndos;
  std::stack<Snapshot<Types...>> mRedos;
};

#include <cstdio>

struct S
{
  int a;
  int b;
};

void isSetS1A(int val)
{
  printf("s1.a was set to %d\n", val);
}

int main()
{
  S s1, s2;
  ExternalManager<int, double> em;

  em.registerCallback(s1.a, &isSetS1A);

  em.set(s1.a, 10);

  em.set(s1.a, 2);

  printf("Undo last change --> ");
  em.undo();

  printf("Redo last change --> ");
  em.redo();

  em.set(s2.a, 20);
  em.set(s1.a, 25);

  em.removeCallback(s1.a);

  em.set(s1.a, 30);

  puts("Now printing value of s1.a");
  isSetS1A(s1.a);

  return s1.a;
}
