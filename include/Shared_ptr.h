#pragma once

// 基类control_block_super
#include <algorithm>
#include <atomic>
#include <utility>
struct control_block_super {
  using u32_int = unsigned int;

  std::atomic<u32_int> strong_ = 1;

  std::atomic<u32_int> weak_ = 0;

  // 成员函数：负责增加引用、弱引用、调用虚函数
  void add_ref() { ++strong_; }
  void add_weak() { ++weak_; }

  void release_ref() {
    if (--strong_ == 0) {
      destroy_res();
      if (weak_ == 0) {
        destroy_self();
      }
    }
  }

  void release_self() {
    if (strong_ == 0 && --weak_ == 0) {
      destroy_self();
    }
  }

  u32_int count() const { return strong_; }

  // 虚函数：使用C/C++的 void* 可以存储任何变量来存储我们真正的模板指针
  virtual void *get_obj() const = 0;
  // 虚函数：释放保持的资源
  virtual void destroy_res() = 0;
  // 虚函数：释放自身
  virtual void destroy_self() = 0;

  virtual ~control_block_super() = default;
};

template <typename T>
static void Default_Delete(T* ptr){
  delete ptr;
}

// 自定义Default删除器 使用重载operator()的无状态删除器
template <typename T> struct Default_Deleter {
  void operator()(T *ptr) const { delete ptr; }
};

// 创建子类来实现多态绑定
template <typename T, typename Deleter>
class control_block_final : public control_block_super {
public:
  using pointer = T *;
  using reference = T &;
  using const_pointer = const T *;
  using const_reference = const T &;

  control_block_final(T *ptr = nullptr, Deleter d = Default_Deleter<T>())
      : ptr_(ptr), del_(std::move(d)) {}

  void destroy_res() override {
    if (ptr_) {
      del_(ptr_);
      ptr_ = nullptr;
    }
  }

  virtual void destroy_self() override { delete this; }

  virtual void *get_obj() const override { return ptr_; }

private:
  T *ptr_;
  Deleter del_;
};

template <typename T> class weak_pointer;

template <typename T> class Shared_Pointer {
  template <typename U> friend class weak_pointer;

public:
  using reference = T &;
  using pointer = T *;
  using const_reference = const T &;
  using const_pointer = const T *;
  using size_type = control_block_super::u32_int;



  template <typename Deleter = Default_Deleter<T>>
  Shared_Pointer(T *ptr, Deleter d = Default_Deleter<T>())
      : cb_(ptr ? new control_block_final<T, Deleter>(ptr, d) : nullptr) {}
  Shared_Pointer(T* ptr):Shared_Pointer(ptr,Default_Delete<T>){}
  Shared_Pointer(const Shared_Pointer &rhs) : cb_(rhs.cb_) {
    if (cb_) {
      cb_->add_ref();
    }
  }

  Shared_Pointer(Shared_Pointer &&rhs) : cb_(rhs.cb_) { rhs.cb_ = nullptr; }

  Shared_Pointer &operator=(Shared_Pointer rhs) {
    swap(rhs);
    return *this;
  }

  Shared_Pointer(const weak_pointer<T> &rhs) : cb_(rhs.cb_) {
    if (cb_ && cb_->count() != 0) {
      cb_->add_ref();
    } else {
      cb_ = nullptr;
    }
  }

  ~Shared_Pointer() { release(); }
  reference operator*() const { return *get(); }
  pointer operator->() const { return get(); }

  pointer get() const { return cb_ ? static_cast<pointer>(cb_->get_obj()) : nullptr; }

  size_type use_count() const { return cb_ ? cb_->count() : 0; }

  template <typename Deleter = Default_Deleter<T>>
  void reset(T *ptr = nullptr, Deleter d = Default_Deleter<T>()) {
    Shared_Pointer(ptr, d).swap(*this);
  }

private:
  // 只需要一个control_block_cb
  control_block_super *cb_;

  void swap(Shared_Pointer &rhs) noexcept {
    using std::swap;
    swap(cb_, rhs.cb_);
  }

  void release() {
    if (cb_) {
      cb_->release_ref();
    }
  }
};

template <typename T> class weak_pointer {
  friend class Shared_Pointer<T>;

public:
  weak_pointer() = default;
  weak_pointer(Shared_Pointer<T> ptr) : cb_(ptr.cb_) {
    if (cb_) {
      cb_->add_weak();
    }
  }
  weak_pointer(const weak_pointer &rhs) : cb_(rhs.cb_) {
    if (cb_) {
      cb_->add_weak();
    }
  }

  weak_pointer(weak_pointer &&rhs) : cb_(std::move(rhs.cb_)) {
    rhs.cb_ = nullptr;
  }
  weak_pointer &operator=(weak_pointer rhs) {
    swap(rhs);
    return *this;
  }
  ~weak_pointer() {
    if (cb_) {
      release();
    }
  }

  Shared_Pointer<T> lock() const {
    if (cb_ && cb_->count() != 0) {
      return Shared_Pointer<T>(*this);
    }
    return Shared_Pointer<T>();
  }

private:
  control_block_super *cb_ = nullptr;

  void release() {
    if (cb_) {
      cb_->release_self();
    }
  }

  void swap(weak_pointer &rhs) noexcept {
    using std::swap;
    swap(cb_, rhs.cb_);
  }
};