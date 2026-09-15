#include <cstdlib>
#include "SharedPtr.h"

#include <cassert>
#include <iostream>

struct TestClass {
    int val;
    static int count;
    TestClass(int v) : val(v) { count++; }
    ~TestClass() { count--; }
};
int TestClass::count = 0;

int main() {
    // default construction
    SharedPtr<int> empty;
    assert(!empty);
    assert(empty.get() == nullptr);
    assert(empty.useCount() == 0);

    // basic construction and dereference
    SharedPtr<int> p(new int(5));
    assert(*p == 5);
    assert(p.useCount() == 1);

    // copy - refcount goes up
    SharedPtr<int> q(p);
    assert(q.useCount() == 2);
    assert(p.useCount() == 2);
    assert(p.get() == q.get());

    // move - refcount stays the same, source becomes empty
    SharedPtr<int> r(std::move(q));
    assert(!q);
    assert(r.useCount() == 2);

    // destruction frees resource when last owner dies
    {
        SharedPtr<TestClass> a(new TestClass(1));
        assert(TestClass::count == 1);
        {
            SharedPtr<TestClass> b(a);
            assert(a.useCount() == 2);
            assert(TestClass::count == 1);
        }
        assert(a.useCount() == 1);
        assert(TestClass::count == 1);
    }
    assert(TestClass::count == 0);

    // copy assignment releases old resource
    SharedPtr<TestClass> x(new TestClass(10));
    SharedPtr<TestClass> y(new TestClass(20));
    assert(TestClass::count == 2);
    y = x;
    assert(TestClass::count == 1); // TestClass(20) freed
    assert(y->val == 10);
    assert(x.useCount() == 2);

    // move assignment
    SharedPtr<TestClass> z(new TestClass(30));
    x = std::move(z);
    assert(!z);
    assert(x->val == 30);
    assert(TestClass::count == 2); 
    // x had refcount 2 (x and y shared TestClass(10)), z had TestClass(30)
    // after move: x = TestClass(30), y = TestClass(10) still alive, TestClass(10) refcount = 1
    assert(y.useCount() == 1);

    // swap
    SharedPtr<int> s1(new int(100));
    SharedPtr<int> s2(new int(200));
    s1.swap(s2);
    assert(*s1 == 200 && *s2 == 100);

    // reset
    SharedPtr<TestClass> f(new TestClass(99));
    assert(TestClass::count >= 1);
    f.reset();
    assert(!f);

    // reset with new pointer
    SharedPtr<TestClass> g(new TestClass(1));
    g.reset(new TestClass(2));
    assert(g->val == 2);

    // self reset
    SharedPtr<TestClass> h(new TestClass(7));
    h.reset(h.get());
    assert(h->val == 7); // must not crash or dangle

    // operator== and !=
    SharedPtr<int> a(new int(1));
    SharedPtr<int> b(a);
    SharedPtr<int> c;
    assert(a == b);
    assert(a != c);

    // makeSharedBasic
    auto ms = makeSharedBasic<int>(42);
    assert(*ms == 42);
    assert(ms.useCount() == 1);

    // makeShared (embedded, bonus)
    auto me = makeShared<int>(99);
    assert(*me == 99);
    auto me2 = me;
    assert(me.useCount() == 2);

    // aliasing constructor (bonus)
    struct Pair { int x = 1; int y = 2; };
    SharedPtr<Pair> owner = makeShared<Pair>();
    SharedPtr<int> alias(owner, &owner->y);
    assert(owner.useCount() == 2);
    assert(*alias == 2);
    owner.reset();
    assert(*alias == 2); // still alive via alias

    std::cout << "all tests passed\n";
    return EXIT_SUCCESS;
}