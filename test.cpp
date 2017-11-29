#define CATCH_CONFIG_MAIN
#include "CVector.hpp"
#include "catch.hpp"

// TODO randomized testing

TEST_CASE("Integers are inserted and read", "[CVector]") {
  int size_for_vector = 1000000;
  auto v1 = CVector<int>(size_for_vector);
  for (int i = 0; i < size_for_vector; i++) {
    v1.set(i, i);
  }
  REQUIRE(v1.get(size_for_vector - 1) == size_for_vector - 1);
  REQUIRE(v1.get(1) == 1);
  REQUIRE(v1.get(0) == 0);
  REQUIRE(v1.get(31) == 31);
  REQUIRE(v1.get(1024) == 1024);
}

// TODO do more snapshot testing
TEST_CASE("Snapshot testing", "[CVector]") {
  int size = 100000;
  auto v1 = CVector<int>(size);
  for (int i = 0; i < size; i++) {
    v1.set(i, i);
  }
  auto v2 = v1.snapshot();
  for (int i = 0; i < size; i++) {
    v2.set(i, 2 * i);
  }

  REQUIRE(v1.get(size - 1) == size - 1);
  REQUIRE(v1.get(1) == 1);
  REQUIRE(v1.get(0) == 0);
  REQUIRE(v1.get(31) == 31);
  REQUIRE(v1.get(1024) == 1024);

  REQUIRE(v2.get(size - 1) == 2 * (size - 1));
  REQUIRE(v2.get(1) == 2 * 1);
  REQUIRE(v2.get(0) == 0);
  REQUIRE(v2.get(31) == 2 * 31);
  REQUIRE(v2.get(1024) == 2 * 1024);
}

TEST_CASE("push_back", "[CVector]") {
  auto v1 = CVector<int>();
  int size = 10000;
  for (int i = 0; i < size; i++) {
    v1.push_back(i);
  }
  REQUIRE(v1.get(size - 1) == size - 1);
  REQUIRE(v1.get(1) == 1);
  REQUIRE(v1.get(0) == 0);
  REQUIRE(v1.get(31) == 31);
  REQUIRE(v1.get(1024) == 1024);
}

TEST_CASE("subscript operator", "[CVector]") {
  int size = 10000;
  auto v1 = CVector<int>(size);
  for (int i = 0; i < size; i++) {
    v1[i] = i;
  }
  REQUIRE(v1.get(size - 1) == size - 1);
  REQUIRE(v1.get(1) == 1);
  REQUIRE(v1.get(0) == 0);
  REQUIRE(v1.get(31) == 31);
  REQUIRE(v1.get(1024) == 1024);
}
