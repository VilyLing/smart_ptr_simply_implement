#pragma once

#include <algorithm>
#include <memory>
#include <utility>
template<typename T>
struct Default_Unique_Deleter{
    void operator()(T* ptr) const {
        delete ptr;
    }
};

template<typename T,typename Deleter = Default_Unique_Deleter<T>> class Unique_ptr{
    public:
        using pointer = T*;
        using ref = T&;
        Unique_ptr(pointer ptr = nullptr,Deleter del = Default_Unique_Deleter<T>()):ptr_(ptr),deleter_(std::move(del)){}
        Unique_ptr(const Unique_ptr& rhs) = delete;
        Unique_ptr(Unique_ptr&& rhs) noexcept:ptr_(rhs.ptr_),deleter_(std::move(rhs.deleter_)){
            rhs.ptr_ = nullptr;
        }
        Unique_ptr& operator=(const Unique_ptr& rhs) = delete;
        Unique_ptr& operator=(Unique_ptr&& rhs) noexcept{
            if (this != & rhs) {
                reset();
                ptr_ = rhs.ptr_;
                deleter_ = std::move(rhs.deleter_);
                rhs.ptr_ = nullptr;
            }
            return *this;
        }
        ~Unique_ptr(){
            if (ptr_) {
                deleter_(ptr_);
            }
        }

        Deleter& get_deleter() noexcept{
            return deleter_;
        }

        const Deleter& get_deleter() const noexcept{
            return deleter_;
        }

        pointer get() const noexcept{
            return ptr_?ptr_:nullptr;
        }

        void swap(Unique_ptr& rhs) noexcept{
            using std::swap;
            swap(ptr_,rhs.ptr_);
            swap(deleter_,rhs.deleter_);
        }

        pointer release(){
            pointer p = ptr_;
            ptr_ = nullptr;
            return p;
        }

        void reset(pointer ptr = nullptr) noexcept{
            pointer old_ = ptr_;
            ptr_ = ptr;
            if (old_) {
                deleter_(old_);
            }
        }

        ref operator*() const {return *ptr_;}
        pointer operator->() const {return ptr_;}
        explicit operator bool() const noexcept{return ptr_ != nullptr;}

        bool operator==(const Unique_ptr& rhs) const noexcept{
            return ptr_ == (rhs.ptr_);
        }

        bool operator!=(const Unique_ptr& rhs) const noexcept{
            return !(*this == rhs);
        }
    private:
        pointer ptr_;
        Deleter deleter_;

 
};