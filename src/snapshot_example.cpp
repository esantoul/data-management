#include "snapshot.hpp"

#include <cstdio>

using namespace dmgmt;

struct Hello
{
  Hello() { puts("Hello"); }
  Hello(const Hello &h)
  {
    (void)h;
    puts("Hello cpy");
  }
  const Hello &operator=(const Hello &h)
  {
    (void)h;
    puts("Hello assign");
    return *this;
  }
  ~Hello() { puts("Goodbye"); }
  bool operator==(const Hello &other) const { return &other == this; }
};

struct Consumer
{
  void consume(const Signature &dat)
  {
    printf("[%s]\n", dat.type().name());
  }
};

int main()
{
  int i = 1;
  int j = 10;
  float f = 10.f;
  Hello h;
  SnapshotGroup grp;
  grp.add(i);
  grp.add(j);
  grp.add(f);
  grp.add(h);
  Consumer c;
  i = j = f = 0;
  SnapshotGroup grp2{std::move(grp)};
  grp2.rollback([&](const Signature &dat) { c.consume(dat); });
  return i + j + f + grp.size() + grp2.size();
}