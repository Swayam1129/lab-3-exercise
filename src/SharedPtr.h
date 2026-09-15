#ifndef SHARED_PTR_HEADER
#define SHARED_PTR_HEADER

#include <cassert>
#include <utility>

// Forward declarations
template <typename T> class SharedPtr;
template <typename T, typename... Args> SharedPtr<T> makeShared(Args&&... args);

class ControlBlockBase {
public:
    ControlBlockBase() : refcount_(1) {} // TODO: implement the default constructor.

    // dtor is virtual, so that we can call derived class's dtor from a ptr to this base class.
    virtual ~ControlBlockBase() {} // TODO: implement the destructor.

    // pure virtual function; must be overriden by derived classes
    virtual void* managedAddress() = 0;

    // Delete copies, which also implicitly deletes moves.
    ControlBlockBase(const ControlBlockBase&) = delete;
    ControlBlockBase& operator=(const ControlBlockBase&) = delete;

    long increment()
    {
        // TODO: increment refcount by 1 and return result.
        assert(refcount_ > 0 && "incrementing a dead control block");
        return ++refcount_;
    }

    long decrement()
    {
        // TODO: decrement refcount by 1 and return result.
        assert(refcount_ > 0 && "decrementing below zero");
        return --refcount_;
    }

    long refCount() const
    {
        // TODO: just return the refcount.
        return refcount_;
    }

private:
    // TODO: add field(s) which both control block types need to have
    long refcount_;
};

// ControlBlock<T> - stores a raw T* and deletes it when refcount hits 0.
template <typename T>
class ControlBlock : public ControlBlockBase {
public:
    explicit ControlBlock(T* ptr) : ptr_(ptr) {}
    ~ControlBlock() override { delete ptr_; }
    void* managedAddress() override { return ptr_; }
private:
    T* ptr_;
};

// ControlBlockEmbedded<T> - embeds T directly inside the control block (bonus).
template <typename T>
class ControlBlockEmbedded : public ControlBlockBase {
public:
    template <typename... Args>
    explicit ControlBlockEmbedded(Args&&... args)
        : obj_(std::forward<Args>(args)...) {}
    ~ControlBlockEmbedded() override {}
    void* managedAddress() override { return &obj_; }
private:
    T obj_;
};

// SharedPtr<T> - shared ownership smart pointer.
template <typename T>
class SharedPtr {
public:
    template <typename U> friend class SharedPtr;
    template <typename U, typename... Args>
    friend SharedPtr<U> makeShared(Args&&... args);

    // Default constructor: empty (both pointers null).
    SharedPtr() noexcept : stored_(nullptr), ctrl_(nullptr) {}

    // Constructor from raw pointer: takes ownership via a new ControlBlock<T>.
    explicit SharedPtr(T* ptr)
        : stored_(ptr),
          ctrl_(ptr ? new ControlBlock<T>(ptr) : nullptr) {}

    // Copy constructor: share ownership, increment refcount.
    SharedPtr(const SharedPtr& other) noexcept
        : stored_(other.stored_), ctrl_(other.ctrl_) {
        if (ctrl_) ctrl_->increment();
    }

    // Copy assignment.
    SharedPtr& operator=(const SharedPtr& other) noexcept {
        if (this != &other)
            SharedPtr<T>(other).swap(*this);
        return *this;
    }

    // Move constructor: steal pointers, leave source empty, refcount unchanged.
    SharedPtr(SharedPtr&& other) noexcept
        : stored_(other.stored_), ctrl_(other.ctrl_) {
        other.stored_ = nullptr;
        other.ctrl_   = nullptr;
    }

    // Move assignment.
    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other)
            SharedPtr<T>(std::move(other)).swap(*this);
        return *this;
    }

    // Aliasing constructor (bonus): share other's control block but use storedPtr.
    template <typename U>
    SharedPtr(const SharedPtr<U>& other, T* storedPtr) noexcept
        : stored_(storedPtr), ctrl_(other.ctrl_) {
        if (ctrl_) ctrl_->increment();
    }

    // Destructor: decrement refcount; free control block (and resource) if last owner.
    ~SharedPtr() {
        if (ctrl_ && ctrl_->decrement() == 0)
            delete ctrl_;
    }

    T& operator*()  const { assert(stored_); return *stored_; }
    T* operator->() const { assert(stored_); return stored_; }
    T* get()        const noexcept { return stored_; }
    long useCount() const noexcept { return ctrl_ ? ctrl_->refCount() : 0L; }
    explicit operator bool() const noexcept { return stored_ != nullptr; }
    bool operator==(const SharedPtr<T>& other) const noexcept {
        return stored_ == other.stored_;
    }

    void swap(SharedPtr<T>& other) noexcept {
        T*                ts = stored_; stored_ = other.stored_; other.stored_ = ts;
        ControlBlockBase* tc = ctrl_;   ctrl_   = other.ctrl_;   other.ctrl_   = tc;
    }

    // Release ownership and become empty.
    void reset() noexcept { SharedPtr<T>().swap(*this); }

    // Release and begin managing other. Guards against self-reset.
    void reset(T* other) {
        if (other == stored_) return;
        SharedPtr<T>(other).swap(*this);
    }

private:
    T*                stored_;
    ControlBlockBase* ctrl_;

    // Private constructor used by makeShared (embedded control block path).
    explicit SharedPtr(T* storedPtr, ControlBlockBase* ctrl) noexcept
        : stored_(storedPtr), ctrl_(ctrl) {}
};

// makeSharedBasic: two allocations (T separately, then ControlBlock).
template <typename T, typename... Args>
SharedPtr<T> makeSharedBasic(Args&&... args) {
    return SharedPtr<T>(new T(std::forward<Args>(args)...));
}

// makeShared: single allocation - T embedded inside ControlBlockEmbedded (bonus).
template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
    auto* ctrl = new ControlBlockEmbedded<T>(std::forward<Args>(args)...);
    return SharedPtr<T>(static_cast<T*>(ctrl->managedAddress()), ctrl);
}

#endif