/**
 * Copyright (C) 2020 Etienne Santoul - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the BSD 2-Clause License
 *
 * You should have received a copy of the BSD 2-Clause License
 * with this file. If not, please visit: 
 * https://github.com/esantoul/data-management
 */

#include <cstdio>
#include <cmath>
#include "static_data_manager.hpp"

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
  dmgmt::StaticDataManager em;
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

  int arg = 5;
  float val = em.call(s1, &S::enlarge, arg);

  em.register_dependency(s1, s1.b); // To test circular dependency check
  em.set(s1.b, 18);

  return val;
}
