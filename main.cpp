#include "CVector.hpp"
#include <chrono>
#include <ctime>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <array>

using namespace std;

const auto s = 25;

int main() {
  auto v = CVector<int>();
  for (int i = 0; i < 34; i++)
    v.push_back(i);
  cout << v.get(32) << endl;
  cout << v.get(0) << endl;
}
