#include <array>
#include <cstddef>
#include <memory>

#include <iostream>

using std::array;
using std::make_shared;
using std::shared_ptr;

// using std::cout;
// using std::endl;

const uint32_t B = 5;
const uint32_t M = 1 << B;

uint32_t local_idx(uint32_t idx, uint32_t lvls) {
  return (idx >> (B * lvls)) & (M - 1);
}

template <class T> class Node {
public:
  virtual shared_ptr<Node<T>> immutable_update(uint32_t idx, uint32_t lvls,
                                               T some) = 0;
  virtual void mutable_update(uint32_t idx, uint32_t lvls, T some) = 0;
  virtual T get(uint32_t idx, uint32_t levels) = 0;
};

template <class T> class Leaf : public Node<T> {
public:
  array<T, M> children;

  explicit Leaf() { children = array<T, M>(); }

  explicit Leaf(array<T, M> c) : children(c) {}

  T get(uint32_t idx, uint32_t lvls) {
    auto l_idx = local_idx(idx, lvls);
    return children[l_idx];
  }

  void mutable_update(uint32_t idx, uint32_t lvls, T some) {
    //    cout << "mutable update on leaf, idx: " << idx << ", lvls: " << lvls
    //         << ", some: " << some << endl;
    auto l_idx = local_idx(idx, lvls);
    children[l_idx] = some;
  }

  shared_ptr<Node<T>> immutable_update(uint32_t idx, uint32_t lvls, T some) {
    // cout << "immutable update on leaf, idx: " << idx << ", lvls: " << lvls
    //      << ", some: " << some << endl;
    shared_ptr<Node<T>> new_leaf = make_shared<Leaf<T>>(Leaf<T>(children));
    new_leaf->mutable_update(idx, lvls, some);
    return new_leaf;
  }
};

template <class T> class Branch : public Node<T> {
public:
  array<shared_ptr<Node<T>>, M> children;
  array<bool, M> read_only;

  explicit Branch(uint32_t lvls) { populate_children(lvls); }

  explicit Branch(array<shared_ptr<Node<T>>, M> c, array<bool, M> ro) {
    children = c;
    read_only = ro;
  }

  explicit Branch(shared_ptr<Node<T>> &old_root) {
    for (uint32_t i = 0; i < M; i++)
      children[i] = old_root;
    for (uint32_t i = 0; i < M; i++)
      read_only[i] = true;
  }

  void populate_children(uint32_t lvls) {
    if (1 < lvls) {
      for (uint32_t i = 0; i < M; i++) {
        children[i] = make_shared<Branch<T>>(lvls - 1);
        read_only[i] = false;
      }
    } else {
      for (uint32_t i = 0; i < M; i++) {
        children[i] = make_shared<Leaf<T>>();
        read_only[i] = false;
      }
    }
  }
  T get(uint32_t idx, uint32_t lvls) {
    auto l_idx = local_idx(idx, lvls);
    return children[l_idx]->get(idx, lvls - 1);
  }

  shared_ptr<Node<T>> immutable_update(uint32_t idx, uint32_t lvls, T some) {
    // cout << "immutable update on branch, idx: " << idx << ", lvls: " << lvls
    //      << ", some: " << some << endl;
    auto l_idx = local_idx(idx, lvls);
    auto new_read_only = array<bool, M>();
    for (uint32_t i = 0; i < M; i++) {
      new_read_only[i] = true;
    }
    read_only[l_idx] = false;
    shared_ptr<Node<T>> updated_child =
        children[l_idx]->immutable_update(idx, lvls - 1, some);
    auto new_children = array<shared_ptr<Node<T>>, M>();
    for (uint32_t i = 0; i < M; i++) {
      if (i != l_idx)
        new_children[i] = children[i];
      else
        new_children[l_idx] = updated_child;
    }
    auto new_branch = make_shared<Branch<T>>(new_children, new_read_only);
    return new_branch;
  }

  void mutable_update(uint32_t idx, uint32_t lvls, T some) {
    auto l_idx = local_idx(idx, lvls);
    if (read_only[l_idx]) {
      children[l_idx] = children[l_idx]->immutable_update(idx, lvls - 1, some);
      read_only[l_idx] = false;
    } else {
      children[l_idx]->mutable_update(idx, lvls - 1, some);
    }
  }
};

template <class T> class CVector {
public:
  explicit CVector<T>() {
    root = nullptr;
    size = 0;
    lvls = 0;
    read_only = false;
  }

  explicit CVector<T>(shared_ptr<Node<T>> r, uint32_t s, uint32_t l, bool ro)
      : root(r), size(s), lvls(l), read_only(ro) {}

  explicit CVector<T>(int s) {
    if (s == 0) {
      root = nullptr;
      size = 0;
      lvls = 0;
      read_only = false;
    } else if (s < M) {
      root = make_shared<Leaf<T>>();
      size = s;
      lvls = 1;
    } else {
      uint32_t level_count = 1;
      uint32_t x = s;
      while (x > 0) {
        x = B >> x;
        level_count++;
      }
      auto r = make_shared<Branch<T>>(level_count - 1);
      root = r;
      size = s;
      lvls = level_count;
      read_only = false;
    }
  }

  CVector snapshot() {
    // read_only = true;
    // cout << "Snap" << endl;
    return CVector(root, size, lvls, false);
  }

  T get(uint32_t idx) { return root->get(idx, lvls - 1); }

  void set(uint32_t idx, T some) {
    if (read_only) {
      root = root->immutable_update(idx, lvls - 1, some);
      read_only = false;
    } else
      root->mutable_update(idx, lvls - 1, some);
  }

  void push_back(T some) {}

private:
  shared_ptr<Node<T>> root;
  uint32_t size;
  uint32_t lvls;
  bool read_only;
};
