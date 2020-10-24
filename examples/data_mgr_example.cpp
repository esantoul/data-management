/**
 * Copyright (C) 2020 Etienne Santoul - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the BSD 2-Clause License
 *
 * You should have received a copy of the BSD 2-Clause License
 * with this file. If not, please visit: 
 * https://github.com/esantoul/data-management
 */

#include "data_manager.hpp"
#include <cmath>
#include <cstdio>

struct S
{
  int a = 0;
  int b = 0;

  constexpr bool operator==(const S &other) { return a == other.a && b == other.b; }

  constexpr int addition_result() const { return a + b; }
  constexpr void vertical_symmetry() { a = -a; }
  float norm() const { return sqrt(a * a + b * b); }
  float enlarge(int length)
  {
    a += length;
    b += length;
    return norm();
  }
};

void meh(S elem)
{
  printf("result of sum in S: %d\n", elem.addition_result());
}

int main()
{
  int i = 0;
  (void)i; // Disable warning unused

  dmgmt::DataManager<S> smgr;

  smgr.set(smgr.get().b, -5);

  smgr.register_callback(
      smgr.get().a, [](int val) { printf("a: %d\n", val); });
  smgr.set(smgr.get().a, 10);

  smgr.register_dependency(smgr.get().a, smgr.get());
  smgr.set(smgr.get().a, 5);

  smgr.register_callback(smgr.get(), meh);
  smgr.set(smgr.get().a, 30);

  smgr.register_dependency(smgr.get().b, smgr.get());
  smgr.set(smgr.get().b, 20);

  // This one should assert fail
  // smgr.register_callback(
  //     i, +[](int val) { printf("what?? %d\n", val); });

  // This one should assert fail
  // smgr.register_dependency(smgr.get(), i);

  smgr.call(smgr.get(), &S::vertical_symmetry);

  return 0;
}