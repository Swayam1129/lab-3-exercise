#ifndef SHARED_PTR_HEADER
#define SHARED_PTR_HEADER

class ControlBlockBase {
public:
    ControlBlockBase(); // TODO: implement the default constructor.

    // dtor is virtual, so that we can call derived class's dtor from a ptr to this base class.
    virtual ~ControlBlockBase(); // TODO: implement the destructor.

    // pure virtual function; must be overriden by derived classes
    virtual void* managedAddress() = 0;

    // Delete copies, which also implicitly deletes moves.
    ControlBlockBase(const ControlBlockBase&) = delete;
    ControlBlockBase& operator=(const ControlBlockBase&) = delete;

    long increment()
    {
        // TODO: increment refcount by 1 and return result.
    }

    long decrement()
    {
        // TODO: decrement refcount by 1 and return result.
    }

    long refCount() const
    {
        // TODO: just return the refcount.
    }

private:
    // TODO: add field(s) which both control block types need to have
};

#endif
