#include <atomic>
#include <memory>
#include <utility>

// 1. 控制块基类（保持简洁）
struct control_block_super {
    std::atomic<unsigned int> strong_{1};
    std::atomic<unsigned int> weak_{0};

    virtual void destroy_resource() = 0; // 释放对象
    virtual void destroy_self() = 0;     // 释放控制块自身
    virtual ~control_block_super() = default;

    // 优化的发布逻辑：
    // 标准做法：只要 strong > 0，weak 计数就隐含加1（代表资源存活）
    // 或者像你这样直接解耦。下面采用更直观的解耦逻辑。
    void release_strong() {
        if (strong_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            destroy_resource();
            if (weak_.load(std::memory_order_acquire) == 0) {
                destroy_self();
            }
        }
    }

    void release_weak() {
        if (weak_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (strong_.load(std::memory_order_acquire) == 0) {
                destroy_self();
            }
        }
    }
};

// 2. 增强型控制块（支持删除器）
template <typename T, typename Deleter = std::default_delete<T>>
class control_block_ptr : public control_block_super, private Deleter {
public:
    explicit control_block_ptr(T* p, Deleter d) : Deleter(std::move(d)), ptr_(p) {}

    void destroy_resource() override {
        static_cast<Deleter&>(*this)(ptr_); // 利用继承实现 EBCO 优化
        ptr_ = nullptr;
    }

    void destroy_self() override { delete this; }

private:
    T* ptr_;
};

// 3. Shared_Pointer 主类
template <typename T>
class Shared_Pointer {
    template <typename U> friend class Shared_Pointer;
    template <typename U> friend class weak_pointer;

public:
    Shared_Pointer() : ptr_(nullptr), cb_(nullptr) {}
    explicit Shared_Pointer(std::nullptr_t) : Shared_Pointer() {}

    // 普通指针构造：explicit 防止隐式转换
    template <typename U>
    explicit Shared_Pointer(U* p) 
        : ptr_(p), cb_(p ? new control_block_ptr<U>(p, std::default_delete<U>{}) : nullptr) {}

    // 支持向上转型的构造函数：Shared_Pointer<Derived> -> Shared_Pointer<Base>
    template <typename U>
    Shared_Pointer(const Shared_Pointer<U>& rhs) : ptr_(rhs.ptr_), cb_(rhs.cb_) {
        if (cb_) cb_->strong_++;
    }

    // 移动构造
    Shared_Pointer(Shared_Pointer&& rhs) noexcept : ptr_(rhs.ptr_), cb_(rhs.cb_) {
        rhs.ptr_ = nullptr;
        rhs.cb_ = nullptr;
    }

    ~Shared_Pointer() { release(); }

    Shared_Pointer& operator=(Shared_Pointer rhs) {
        swap(rhs);
        return *this;
    }

    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    T* get() const { return ptr_; }
    unsigned int use_count() const { return cb_ ? cb_->strong_.load() : 0; }

    void swap(Shared_Pointer& rhs) noexcept {
        using std::swap;
        swap(ptr_, rhs.ptr_);
        swap(cb_, rhs.cb_);
    }

private:
    T* ptr_;
    control_block_super* cb_;

    void release() {
        if (cb_) cb_->release_strong();
    }
    
    // 供 weak_pointer 升级使用的内部构造
    Shared_Pointer(T* p, control_block_super* c) : ptr_(p), cb_(c) {
        if (cb_) cb_->strong_++;
    }
};

// 4. 实现更加健壮的 weak_pointer
template <typename T>
class weak_pointer {
public:
    weak_pointer() : ptr_(nullptr), cb_(nullptr) {}

    template <typename U>
    weak_pointer(const Shared_Pointer<U>& s) : ptr_(s.ptr_), cb_(s.cb_) {
        if (cb_) cb_->weak_++;
    }

    Shared_Pointer<T> lock() const {
        // 原子检查并增加引用计数是重点
        if (!cb_) return {};
        
        unsigned int count = cb_->strong_.load(std::memory_order_relaxed);
        do {
            if (count == 0) return {};
        } while (!cb_->strong_.compare_exchange_weak(count, count + 1, 
                                                   std::memory_order_acq_rel, 
                                                   std::memory_order_relaxed));
        
        // 注意：这里手动增加了 strong_ 计数，所以调用特殊的私有构造
        return Shared_Pointer<T>(ptr_, cb_, true); 
    }

private:
    T* ptr_;
    control_block_super* cb_;
};