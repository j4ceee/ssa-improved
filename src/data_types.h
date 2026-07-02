#pragma once

#include <cstdint>

// Ltl2String ----------------------------------------------------------------------------------------------------------
struct Ltl2String
{
    char* mData;
    uint16_t mSize;
    uint16_t mCapacity;
    void* mAllocator;

    [[nodiscard]] const char* c_str() const { return mData ? mData : ""; }
    [[nodiscard]] int32_t length() const { return mSize; }
    [[nodiscard]] bool empty() const { return mSize == 0; }
};
static_assert(sizeof(Ltl2String) == 12);


// Vec3 ----------------------------------------------------------------------------------------------------------------
struct Vec3
{
    float x, y, z;

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }
};
static_assert(sizeof(Vec3) == 12);


// List ----------------------------------------------------------------------------------------------------------------

struct ListNodeBase
{
    ListNodeBase* mPrev;
    ListNodeBase* mNext;
};

static_assert(sizeof(ListNodeBase) == 8);

template <typename T>
struct ListNode : ListNodeBase
{
    T value;
};

template <typename T>
struct List
{
    ListNodeBase mHead;
    void* mAllocator;

    // prevent list from being copied
    List(const List&) = delete;
    List& operator=(const List&) = delete;
    List(List&&) = delete;
    List& operator=(List&&) = delete;

    [[nodiscard]] bool empty() const {
        return !mHead.mNext || mHead.mNext == &mHead;
    }

    struct Iterator
    {
        ListNodeBase* cur;
        T& operator*() const { return static_cast<ListNode<T>*>(cur)->value; }
        T* operator->() const { return &static_cast<ListNode<T>*>(cur)->value; }

        Iterator& operator++()
        {
            cur = cur->mNext;
            return *this;
        }

        bool operator!=(const Iterator& o) const { return cur != o.cur; }
    };

    struct ConstIterator
    {
        const ListNodeBase* cur;
        const T& operator*() const { return static_cast<const ListNode<T>*>(cur)->value; }
        const T* operator->() const { return &static_cast<const ListNode<T>*>(cur)->value; }

        ConstIterator& operator++()
        {
            cur = cur->mNext;
            return *this;
        }

        bool operator!=(const ConstIterator& o) const { return cur != o.cur; }
    };

    Iterator begin() { return Iterator{mHead.mNext}; }
    Iterator end() { return Iterator{&mHead}; }
    ConstIterator begin() const { return ConstIterator{mHead.mNext}; }
    ConstIterator end() const { return ConstIterator{&mHead}; }
};

static_assert(sizeof(List<int>) == 12);
