#include <algorithm>
#include <gtest/gtest.h>
#include <iostream>
#include <ostream>
#include "Unique_ptr.h"
#include "Shared_ptr.h"
class Tracker {
public:
    static int alive_count;
    int id;
    Tracker(int i = 0) : id(i) { ++alive_count; }
    ~Tracker() { --alive_count; }
    Tracker(const Tracker&) = delete;
    Tracker& operator=(const Tracker&) = delete;
};
int Tracker::alive_count = 0;



TEST(UniqueTest, Default_Construct){
    Unique_ptr<int> up;
    EXPECT_EQ(up.get(), nullptr);
    EXPECT_FALSE(up);
}

TEST(UniqueTest, Construct_From_Raw){
    Unique_ptr<Tracker> p(new Tracker(42));
    EXPECT_NE(nullptr, p.get());
    EXPECT_TRUE(p);
    EXPECT_EQ(p->id, 42);
}

TEST(UniqueTest, Destructor_Release_Resource){
    EXPECT_EQ(Tracker::alive_count, 0);
    {
        Unique_ptr<Tracker> p = new Tracker(42);
        EXPECT_EQ(Tracker::alive_count, 1);
    }
    EXPECT_EQ(Tracker::alive_count, 0);
    
}

TEST(UniqueTest, Move_Constructor){
    Unique_ptr<Tracker> p1(new Tracker(5));
    Unique_ptr<Tracker> p2(std::move(p1));
    EXPECT_EQ(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(p2->id, 5);
}

TEST(UniqueTest, Move_Assignment){
    Unique_ptr<Tracker> p1(new Tracker(5));
    Unique_ptr<Tracker> p2;
    p2 = std::move(p1);
    EXPECT_EQ(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_EQ(p2->id, 5);
}

TEST(UniquePtrTest, Release) {
    Unique_ptr<Tracker> p(new Tracker(99));
    Tracker* raw = p.release();
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_NE(raw, nullptr);
    delete raw;  // 手动释放
}

TEST(UniquePtrTest, Reset) {
    Unique_ptr<Tracker> p(new Tracker(1));
    p.reset(new Tracker(2));
    EXPECT_EQ(p->id, 2);
    p.reset();  // 释放资源，变为空
    EXPECT_EQ(p.get(), nullptr);
}
template<typename T>
struct CustomDeleter{
    std::ostream& out;
    inline static int count = 0;
    CustomDeleter(std::ostream& os = std::cout):out(os){}
    void operator()(T* ptr){
        ++count;
        delete ptr;
    }

    void print() const{
        out << "Here is the CustomDeleter" << std::endl;
    }
};

TEST(UniquePtrTest, Custom_deleter){
    int * raw = new int(100);
    Unique_ptr<int,CustomDeleter<int>> p(raw,CustomDeleter<int>(std::cerr));
    EXPECT_EQ(*p, 100);
    p.reset();
    EXPECT_EQ(CustomDeleter<int>::count , 1);
}


TEST(SharedPointerTest, DefaultConstruct) {
    Shared_Pointer<int> sp;
    EXPECT_EQ(sp.get(), nullptr);
    EXPECT_EQ(sp.use_count(), 0);
    EXPECT_FALSE(sp);
}

TEST(SharedPointerTest, Copy_Construct){
    Shared_Pointer<int> sp1(new int(1));
    Shared_Pointer<int> sp2(sp1);
    EXPECT_EQ(sp1.use_count(), 2);
    *sp2 = 6;
    EXPECT_EQ(*sp1, 6);
    sp2.reset();
    EXPECT_EQ(sp1.use_count(),1);
    EXPECT_EQ(sp2,nullptr);
}

TEST(SharedPointerTest, Copy_assignment_operator){
    Shared_Pointer<int> sp1(new int (30));
    Shared_Pointer<int> sp2 = sp1;
    EXPECT_EQ(sp1.use_count(),2);
    *sp2 = 12;
    EXPECT_EQ(*sp1, 12);
}

TEST(SharedPointerTest, Move_Constructor){
    Shared_Pointer<int> sp1(new int(21));
    Shared_Pointer<int> sp2(std::move(sp1));
    EXPECT_EQ(sp2.use_count(), 1);
    EXPECT_EQ(sp1, nullptr);
    EXPECT_EQ(sp1.use_count(), 0);
}

TEST(SharedPointerTest, CopyAssignmentOperator) {
    Shared_Pointer<int> sp1(new int(42));
    Shared_Pointer<int> sp2;
    sp2 = sp1;
    EXPECT_EQ(sp1.use_count(), 2);
    EXPECT_EQ(sp2.use_count(), 2);
    EXPECT_EQ(*sp2, 42);
}

TEST(SharedPointerTest, MoveAssignmentOperator) {
    Shared_Pointer<int> sp1(new int(100));
    Shared_Pointer<int> sp2;
    sp2 = std::move(sp1);
    EXPECT_EQ(sp1.get(), nullptr);
    EXPECT_EQ(sp2.use_count(), 1);
    EXPECT_EQ(*sp2, 100);
}

TEST(SharedPointerTest, Reset) {
    Shared_Pointer<int> sp(new int(10));
    sp.reset(new int(20));
    EXPECT_EQ(*sp, 20);
    sp.reset();
    EXPECT_EQ(sp.get(), nullptr);
    EXPECT_EQ(sp.use_count(), 0);
}

TEST(SharedPointerTest, CompareWithNullptr) {
    Shared_Pointer<int> sp;
    EXPECT_TRUE(sp == nullptr);
    EXPECT_TRUE(nullptr == sp);
    EXPECT_FALSE(sp != nullptr);
    sp.reset(new int(5));
    EXPECT_TRUE(sp != nullptr);
    EXPECT_FALSE(sp == nullptr);
}

TEST(WeakPointerTest, Lock) {
    Shared_Pointer<int> sp(new int(7));
    weak_pointer<int> wp(sp);
    auto sp2 = wp.lock();
    EXPECT_EQ(sp2.use_count(), 2);
    EXPECT_EQ(*sp2, 7);
    sp.reset();
    auto sp3 = wp.lock();
    EXPECT_EQ(sp3.get(), nullptr);
}

struct Node {
    int value;
    Shared_Pointer<Node> next;
    weak_pointer<Node> weak_prev;  // 用弱引用打破循环
    ~Node() { /* 便于观察 */ }
};
TEST(WeakPointerTest, BreakCycle) {
    auto a = Shared_Pointer<Node>(new Node{1});
    auto b = Shared_Pointer<Node>(new Node{2});
    a->next = b;
    b->weak_prev = a;  // 弱引用，不会增加引用计数
    // 此时 a 和 b 都能正确析构，无内存泄漏
}