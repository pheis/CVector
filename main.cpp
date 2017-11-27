#include "CVector.hpp"
#include <iostream>

#include <array>

using namespace std;

int main() {
  auto v = CVector<int>(25);
  for (int i = 0; i < 25; i++) {
    v.set(i, i);
  }
  v.print_contents();
  // auto v2 = v.snapshot();
  // for (int i = 50; i < 100; i++) {
  //  v.set(i, i);
  //}
  // v.print_contents();
  // cout << endl;
  // cout << endl;
}
