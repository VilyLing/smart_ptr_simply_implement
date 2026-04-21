#pragma once

#include <concepts>
#include <utility>
//use OOP 
template <typename T> class control_block_super {
public:
  using u32 = unsigned int;
  using reference = T &;
  using pointer = T *;

  control_block_super(T *ptr = nullptr, u32 count = 1)
      : ptr_(ptr), count_(count) {}

  void add_ref() { ++count_; }
  control_block_super &operator++() {
    ++count_;
    return *this;
  }
  control_block_super &operator--() {
    --count_;
    return *this;
  }

  bool operator()() const { return ptr_; }
  reference operator*() const { return *ptr_; }
  pointer operator->() const { return ptr_; }
  u32 count() const { return count_; }

protected:
  virtual ~control_block_super() = default;

  virtual void release() {
    if (--count_ == 0) {
      desres();
      destory();
    }
  }
  virtual void desres() { delete ptr_; }
  virtual void destory() { delete this; }

private:
  T *ptr_;
  u32 count_;
};

template <typename Del, typename T>
concept IsValidControlBlock = std::derived_from<Del, control_block_super<T>>;

template <typename T> class Shared_Pointer {
public:
  using pointer = T *;
  using ref = T &;
  template <typename Deleter = control_block_super<T>>
    requires IsValidControlBlock<Deleter, T>
  Shared_Pointer(T *ptr = nullptr) : contrl_(ptr ? new Deleter(ptr) : nullptr) {}
  Shared_Pointer(const Shared_Pointer &rhs) : contrl_(rhs.contrl_) {
    if (contrl_) {
      contrl_->add_ref();
    }
  }
  Shared_Pointer &operator=(const Shared_Pointer &rhs) {
    Shared_Pointer temp(rhs);
    swap(temp);
    return *this;
  }
  Shared_Pointer(Shared_Pointer &&rhs) noexcept : contrl_(std::move(rhs.contrl_)) {
    rhs.contrl_ = nullptr;
  }
  Shared_Pointer &operator=(Shared_Pointer &&rhs) {
    if (this != &rhs) {
      swap(rhs);
      rhs.contrl_ = nullptr;
    }
    return *this;
  }

  ~Shared_Pointer() {
    if (contrl_) {
      contrl_->release();
    }
  }

  ref operator*() const { return *contrl_->operator(); }
  pointer operator->() const { return contrl_->operator(); }
  auto use_count() const { return contrl_ ? contrl_->count() : 0; }
  void reset(T *ptr = nullptr) {
    if (is_empty() || contrl_->operator->() != ptr) {
      Shared_Pointer(ptr).swap(*this);
    }
  }
  template <typename Deleter = control_block_super<T>>
    requires IsValidControlBlock<Deleter, T>

  void reset(Deleter *ctr) {
    if (contrl_ != ctr) {
      Shared_Pointer temp;
      temp.contrl_ = ctr;
      this->swap(temp);
    }
  }
  bool is_empty() const { return !contrl_; }

private:
  control_block_super<T> *contrl_;

  void swap(Shared_Pointer &rhs) noexcept {
    using std::swap;
    swap(contrl_, rhs.contrl_);
  }
};