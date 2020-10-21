#include <functional>
#include <memory>
#include <cassert>

class PolyFunDataBase
{
public:
  virtual ~PolyFunDataBase() {}

  virtual void operator()(const void *) const = 0;

  virtual const std::type_info &type() const = 0;

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

  void operator()(const void *data_ptr) const
  {
    mFun(*static_cast<const El_t *>(data_ptr));
  }

  const std::type_info &type() const { return typeid(El_t); }

private:
  std::function<void(const El_t &)> mFun;
};

class PolyFun
{
public:
  template <typename El_t>
  PolyFun(const std::function<void(const El_t &)> &fun)
      : mFunData{new PolyFunData<El_t>(fun)}
  {
  }

  template <typename El_t>
  void operator()(const El_t &data)
  {
    if (typeid(El_t) == mFunData->type())
      (*mFunData.get())(&data);
    else
      assert(false);
  }

  template <typename Dat_t, typename Functor_t>
  static std::function<void(const Dat_t &)> fmt(const Functor_t &f)
  {
    return {f};
  }

private:
  std::unique_ptr<PolyFunDataBase> mFunData;
};
