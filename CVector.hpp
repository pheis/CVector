#include <array>
#include <cstddef>
#include <iostream>
#include <memory>

using std::array;
using std::make_shared;
using std::shared_ptr;

using std::cout;
using std::endl;

const uint32_t B = 5;
const uint32_t M = 1 << B;

uint32_t local_idx(const uint32_t idx, const uint32_t lvls) {
  return (idx >> (B * lvls)) & (M - 1);
}

uint32_t calculate_min_level_count(const uint32_t size) {
  uint32_t lvl_count = 0;
  for (uint32_t x = size; x > 0; x = x >> B, lvl_count++) {
  }
  return lvl_count;
}

uint32_t get_leaf(const uint32_t idx) { return idx / M; };

template <class T> class Node {
public:
  virtual shared_ptr<Node<T>>
  immutable_update(const uint32_t idx, const uint32_t lvls, const T &some) = 0;
  virtual void mutable_update(const uint32_t idx, const uint32_t lvls,
                              const T &some) = 0;
  virtual T get(const uint32_t idx, const uint32_t levels) const = 0;
};

template <class T> class Leaf : public Node<T> {
public:
  array<T, M> children;

  explicit Leaf() { children = array<T, M>(); }

  explicit Leaf(array<T, M> c) : children(c) {}

  T get(const uint32_t idx, const uint32_t lvls) const {
    auto l_idx = local_idx(idx, lvls);
    return children[l_idx];
  }

  void mutable_update(const uint32_t idx, const uint32_t lvls, const T &some) {
    // cout << "mutable update on leaf, idx: " << idx << ", lvls: " << lvls
    //     << ", some: " << some << endl;
    auto l_idx = local_idx(idx, lvls);
    children[l_idx] = some;
  }

  shared_ptr<Node<T>> immutable_update(const uint32_t idx, const uint32_t lvls,
                                       const T &some) {
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

  // partial populate
  explicit Branch(const uint32_t s, const uint32_t lvls) {
    partial_populate(s, lvls);
  }
  // full populate
  explicit Branch(const uint32_t lvls) { full_populate(lvls); }
  // this is the default
  explicit Branch(const array<shared_ptr<Node<T>>, M> &c,
                  const array<bool, M> &ro) {
    children = c;
    read_only = ro;
  }
  // this one is called from push_back;
  explicit Branch(shared_ptr<Node<T>> &old_root, bool ro) {
    children[0] = old_root;
    read_only[0] = ro;
  }

  void partial_populate(const uint32_t s, const uint32_t lvls) {
    auto last_idx = local_idx(s - 1, lvls);
    if (lvls > 1) {
      for (uint32_t i = 0; i < last_idx; i++) {
        children[i] = make_shared<Branch<T>>(lvls - 1);
        read_only[i] = false;
      }
      children[last_idx] = make_shared<Branch<T>>(s, lvls - 1);
    } else {
      for (uint32_t i = 0; i <= last_idx; i++) {
        children[i] = make_shared<Leaf<T>>();
        read_only[i] = false;
      }
    }
  }

  void full_populate(const uint32_t lvls) {
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

  T get(const uint32_t idx, const uint32_t lvls) const {
    auto l_idx = local_idx(idx, lvls);
    return children[l_idx]->get(idx, lvls - 1);
  }

  void fix_null_child(const uint32_t idx, const uint32_t lvls) {
    auto l_idx = local_idx(idx, lvls);
    if (children[l_idx] == nullptr) {
      if (lvls == 1) {
        children[l_idx] = make_shared<Leaf<T>>();
      } else {
        children[l_idx] = make_shared<Branch<T>>(idx + 1, lvls - 1);
      }
    }
  }

  shared_ptr<Node<T>> immutable_update(const uint32_t idx, const uint32_t lvls,
                                       const T &some) {
    // cout << "immutable update on branch, idx: " << idx << ", lvls: " << lvls
    //     << ", some: " << some << endl;
    auto l_idx = local_idx(idx, lvls);
    // this check is needed for push_back
    fix_null_child(idx, lvls);
    //
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

  void mutable_update(const uint32_t idx, const uint32_t lvls, const T &some) {
    // check needed for push_back()
    fix_null_child(idx, lvls);
    // cout << "mutable update on branch, idx: " << idx << ", lvls: " << lvls
    //     << ", some: " << some << endl;
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

  explicit CVector<T>(const int s) {
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
      uint32_t level_count = calculate_min_level_count(s);
      // uint32_t x = s;
      // while (x > 0) {
      //   x = B >> x;
      //   level_count++;
      // }
      auto r = make_shared<Branch<T>>(s, level_count - 1);
      root = r;
      size = s;
      lvls = level_count;
      read_only = false;
    }
  }

  CVector snapshot() {
    // cout << "Snap" << endl;
    read_only = true;
    return CVector(root, size, lvls, true);
  }

  T get(const uint32_t idx) const { return root->get(idx, lvls - 1); }

  void set(const uint32_t idx, const T &some) {
    if (read_only) {
      root = root->immutable_update(idx, lvls - 1, some);
      read_only = false;
    } else
      root->mutable_update(idx, lvls - 1, some);
  }

  void push_back(T &some) {
    if (size == 0) {
      root = make_shared<Leaf<T>>();
      size++;
      lvls++;
      set(0, some);
    } else {
      auto new_size = size + 1;
      auto idx = size; // idx where we insert is the old_size
      auto same_level = ((1 << (B * lvls)) != idx);
      if (same_level) {
        // cout << "was same level" << endl;
        size = new_size;
        set(idx, some);
      } else {
        // cout << "was not same level" << endl;
        size = new_size;
        lvls = lvls + 1;
        root = make_shared<Branch<T>>(root, read_only);
        set(idx, some);
      }
    }
  }

  int get_levels() const { return lvls; }

  int get_size() const { return size; }

  struct Proxy {
    CVector &v;
    int idx;
    Proxy(CVector &v, int idx) : v(v), idx(idx) {}

    operator T() {
      // std::cout << "reading\n"; return a.data[index];
      return v.get(idx);
    }

    // T &operator=(const T &some) {
    Proxy &operator=(T &some) {
      v.set(idx, some);
      return *this;
    }
  };

  Proxy operator[](const int idx) { return Proxy(*this, idx); }

  // TODO
  // map -- in a some bulk update style
  // mapWithIndex
  // filter
  // reduce
  // concat in log_2(n)

private:
  shared_ptr<Node<T>> root;
  uint32_t size;
  uint32_t lvls;
  bool read_only;
};
