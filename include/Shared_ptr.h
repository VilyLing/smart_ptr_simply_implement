#pragma once

#include <iostream>
#include <utility>

//控制块的基类
struct control_block_super{
    using u32  = unsigned int;
    u32 count_ = 1;
    u32 weak_ = 0;

    u32 count() const{ return count_;}
    u32 weak() const {return weak_;}
    void release_strong(){
        if (--count_ == 0) {
            destroy_res();
        }
        if (weak_ == 0) {
            destroy_block();
        }
    }

    void release_weak(){
        if (count_ == 0 && --weak_ == 0) {
            destroy_block();
        }
    }

    void add_ref(){++count_;}
    void add_weak(){++weak_;}

    virtual void destroy_res() = 0;
    virtual void destroy_block() = 0;
    virtual ~control_block_super() = default;
};

//自定义的默认删除器
template<typename T>
void Default_Deleter(T* ptr){
    std::cout << "Delete the pointer" << std::endl;
    delete ptr;
}

template<typename T>
struct Default_Deleter_struct{
    void operator()(T* ptr)const{
        std::cout <<"Delete in struct point"<< std::endl;
        delete ptr;
    }
};

//自定义的控制块子类来完成擦除
template<typename T,typename Deleter = decltype(&Default_Deleter<T>)>
class control_block_final final : public control_block_super{
    public:
        control_block_final(T* ptr,Deleter del):ptr_(ptr),del(std::move(del)){}

        void destroy_res() override{
            del(ptr_);
            ptr_ = nullptr;
        }

        void destroy_block() override{
            delete this;
        }
    private:
        T* ptr_;
        Deleter del;
};

template<typename T>
class Shared_ptr{
    public:
        using pointer = T*;
        using reference = T&;
        using const_pointer = const T*;
        using const_reference = const T&;

        explicit Shared_ptr(T* ptr = nullptr):ptr_(ptr),cb_(nullptr){
            if (ptr_) {
                cb_ = new control_block_final<T>(ptr_,&Default_Deleter);
            }
        }

        template<typename Deleter = decltype(&Default_Deleter<T>)>
        Shared_ptr(T* ptr,Deleter del):ptr_(ptr),cb_(nullptr){
            if (ptr_) {
                cb_ = new control_block_final<T>(ptr,std::move(del));
            }
        }

        Shared_ptr(const Shared_ptr& rhs):ptr_(rhs.ptr_),cb_(rhs.cb_){
            if(cb_)
                cb_->add_ref();
        }

        

        Shared_ptr& operator=(Shared_ptr rhs){
            swap(rhs);
            return *this;
        }



        ~Shared_ptr(){
            release();
        }

        pointer get() const {return *ptr_;}
        
        reference operator*(){return *ptr_;}
        pointer operator->(){return ptr_;}

        const_reference operator*() const {return *ptr_;}
        const_pointer operator->() const {return ptr_;}
    private:
        T* ptr_;
        control_block_super* cb_;

        void release(){
            if (cb_) {
                cb_->release_strong();
            }
        }

        void swap(Shared_ptr& rhs) noexcept{
            using std::swap;
            swap(ptr_,rhs.ptr_);
            swap(cb_,rhs.cb_);
        }
};