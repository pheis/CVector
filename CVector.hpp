#include <array>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>

// M == branching factor,
// tells how many children there are for a branch and a leaf

const uint32_t B = 4;
const uint32_t M = 1 << B;

// gotta be fast.
// same thing could be achieved with / and %,
// but I believe this is faster.
uint32_t local_idx(uint32_t idx, uint32_t lvls) {
  return (idx >> (B * lvls)) & (M - 1);
}

using std::array;
using std::cout;
using std::endl;
using std::make_shared;
using std::make_unique;
using std::move;
using std::mutex;
using std::pair;
using std::shared_ptr;
using std::string;
using std::unique_ptr;

template <class T> class Node {
public:
  virtual shared_ptr<Node<T>> immutable_set(uint32_t idx, uint32_t levels,
                                            T some) = 0;
  virtual T get(uint32_t idx, uint32_t levels) = 0;
  virtual void destructive_set(uint32_t idx, uint32_t lvls, T some) = 0;
};

template <class T> class Branch : public Node<T> {
public:
  //// This one is executed when path copying happens.
  explicit Branch(array<shared_ptr<Node<T>>, M> c, uint32_t idx) {
    snapshot_flags = array<bool, M>();

    for (uint32_t i = 0; i < M; i++) {
      snapshot_flags[i] = true;
    }
    snapshot_flags[idx] = false;

    contents = c;
  }

  explicit Branch(shared_ptr<Node<T>> &old_root) {
    snapshot_flags = array<bool, M>();
    for (uint32_t i = 0; i < M; i++) {
      snapshot_flags[i] = true;
    }
    snapshot_flags[0] = false;
    contents = array<shared_ptr<Node<T>>, M>();
    for (uint32_t i = 0; i < M; i++)
      contents[i] = old_root;
  }

  T get(uint32_t idx, uint32_t lvls) {
    T some = contents[local_idx(idx, lvls)]->get(idx, lvls - 1);
    return some;
  }

  shared_ptr<Node<T>> immutable_set(uint32_t idx, uint32_t lvls, T some) {
    uint32_t l_idx = local_idx(idx, lvls);
    for (uint32_t u = 0; u < M; u++) {
      snapshot_flags[u] = true;
    }
    snapshot_flags[l_idx] = false;
    shared_ptr<Branch<T>> new_branch = make_shared<Branch<T>>(contents, l_idx);
    new_branch->contents[l_idx] = contents[l_idx]->set(idx, lvls - 1, some);
    return new_branch;
  }

  void destructive_set(uint32_t idx, uint32_t lvls, T some) {
    // this one is the tricky one. This is the default also.
    auto l_idx = local_idx(idx, lvls);
    if (read_only[l_idx]) {
      shared_ptr<Node<T>> new_child = contents[l_idx]->set(idx, lvls - 1, some);
      read_only[l_idx] = false;
    } else {
      contents[l_idx]->destructive_set(idx, lvls - 1, some);
    }
  }

  array<bool, M> read_only;
  array<shared_ptr<Node<T>>, M> contents;
};

template <class T> class Leaf : public Node<T> {
public:
  explicit Leaf() { contents = array<T, M>(); }
  explicit Leaf(array<T, M> c) : contents(c) {}

  T get(uint32_t idx, uint32_t lvls) {
    T some = contents[local_idx(idx, lvls)];
    return some;
  }

  shared_ptr<Node<T>> immutable_set(uint32_t idx, uint32_t lvls, T some) {
    auto new_leaf = make_shared<Leaf<T>>(Leaf<T>(contents));
    new_leaf->contents[local_idx(idx, lvls)] = some;
    return new_leaf;
  }

  void destructive_set(uint32_t idx, uint32_t lvls, T some) {
    contents[local_idx(idx, lvls)] = some;
  }

  array<T, M> contents;
};

template <class T> class CVector {
public:
  explicit CVector<T>() {
    root = nullptr;
    size = 0;
    lvls = 0;
    snapshot_flag = false;
  }

private:
  shared_ptr<Node<T>> root;
  uint32_t size;
  uint32_t lvls;
  bool snapshot_flag;
};
