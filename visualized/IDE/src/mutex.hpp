// Based on https://clang.llvm.org/docs/ThreadSafetyAnalysis.html
#pragma once

#include <mutex>

// Enable thread safety attributes only with clang. Exclude Apple Clang since it is too old.
#if defined(__clang__) && !defined(SWIG) && !defined(__apple_build_version__)
#define THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x) // no-op
#endif

#define CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(capability(x))

#define SCOPED_CAPABILITY \
  THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#define GUARDED_BY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define PT_GUARDED_BY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))

#define ACQUIRED_BEFORE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(__VA_ARGS__))

#define ACQUIRED_AFTER(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(__VA_ARGS__))

#define REQUIRES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))

#define REQUIRES_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(__VA_ARGS__))

#define ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))

#define ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(__VA_ARGS__))

#define RELEASE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))

#define RELEASE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(__VA_ARGS__))

#define TRY_ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(__VA_ARGS__))

#define TRY_ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(__VA_ARGS__))

#define EXCLUDES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))

#define ASSERT_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))

#define ASSERT_SHARED_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))

#define RETURN_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#define NO_THREAD_SAFETY_ANALYSIS \
  THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)

/// Use this class instead of std::mutex
class CAPABILITY("mutex") Mutex {
  std::mutex mutex;

public:
  void lock() ACQUIRE() {
    mutex.lock();
  }

  bool try_lock() TRY_ACQUIRE(true) {
    return mutex.try_lock();
  }

  void unlock() RELEASE() {
    mutex.unlock();
  }

  const Mutex &operator!() const { return *this; }
};

/// Use this class instead of std::lock_guard and std::unique_lock
class SCOPED_CAPABILITY LockGuard {
  Mutex &mutex;
  bool locked;

public:
  LockGuard(Mutex &mutex_) ACQUIRE(mutex_) : mutex(mutex_) {
    mutex.lock();
    locked = true;
  }
  void lock() ACQUIRE() {
    mutex.lock();
    locked = true;
  }
  void unlock() RELEASE() {
    mutex.unlock();
    locked = false;
  }
  ~LockGuard() RELEASE() {
    if(locked)
      mutex.unlock();
  }
};
