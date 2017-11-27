#include "CVector.hpp"
#include <iostream>

#include <array>

using namespace std;

const auto s = 25;

int main() {
  auto v = CVector<int>(s);
  for (int i = 0; i < s; i++) {
    v.set(i, i);
  }

  auto v2 = v.snapshot();

  for (int i = 0; i < s; i++) {
    v.set(i, i + 100);
  }

  cout << "snapshot:" << endl;

  for (int i = 0; i < s; i++) {
    auto x = v2.get(i);
    cout << x << endl;
  }

  cout << "mutated:" << endl;

  for (int i = 0; i < s; i++) {
    auto x = v.get(i);
    cout << x << endl;
  }
  // v.print_contents();
  // auto v2 = v.snapshot();
  // for (int i = 50; i < 100; i++) {
  //  v.set(i, i);
  //}
  // v.print_contents();
  // cout << endl;
  // cout << endl;
}
