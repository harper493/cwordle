#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>
#include <thread>
#include <chrono>

/**
 * A lightweight spinlock implementation using atomic operations.
 * 
 * Features:
 * - Uses std::atomic for thread safety
 * - Implements RAII with lock_guard support
 * - Configurable spin count before yielding
 * - Memory ordering optimized for performance
 * - No heap allocations
 */
class spinlock {
private:
    std::atomic<bool> locked_{false};
    static constexpr int SPIN_COUNT = 1000;  // Spin this many times before yielding

public:
    spinlock() = default;
    ~spinlock() = default;
    
    // Delete copy constructor and assignment
    spinlock(const spinlock&) = delete;
    spinlock& operator=(const spinlock&) = delete;
    
    /**
     * Try to acquire the lock.
     * @return true if lock was acquired, false otherwise
     */
    bool try_lock() noexcept {
        bool expected = false;
        return locked_.compare_exchange_strong(expected, true, 
                                             std::memory_order_acquire,
                                             std::memory_order_relaxed);
    }
    
    /**
     * Acquire the lock, spinning if necessary.
     * Uses exponential backoff to reduce contention.
     */
    void lock() noexcept {
        int spin_count = 0;
        
        while (!try_lock()) {
            // Spin for a while before yielding
            if (spin_count < SPIN_COUNT) {
                spin_count++;
                // CPU pause hint to reduce power consumption
                #if defined(__x86_64__) || defined(__i386__)
                    __builtin_ia32_pause();
                #elif defined(__aarch64__)
                    __asm__ __volatile__("yield" ::: "memory");
                #endif
            } else {
                // Yield to other threads after spinning
                std::this_thread::yield();
                spin_count = 0;  // Reset spin count
            }
        }
    }
    
    /**
     * Release the lock.
     */
    void unlock() noexcept {
        locked_.store(false, std::memory_order_release);
    }
    
    /**
     * Check if the lock is currently held.
     * Note: This is only useful for debugging, not for synchronization.
     */
    bool is_locked() const noexcept {
        return locked_.load(std::memory_order_relaxed);
    }
};

/**
 * RAII wrapper for spinlock using std::lock_guard pattern.
 */
template<typename Lock>
class spinlock_guard {
private:
    Lock& lock_;
    
public:
    explicit spinlock_guard(Lock& lock) : lock_(lock) {
        lock_.lock();
    }
    
    ~spinlock_guard() {
        lock_.unlock();
    }
    
    // Delete copy constructor and assignment
    spinlock_guard(const spinlock_guard&) = delete;
    spinlock_guard& operator=(const spinlock_guard&) = delete;
};

// Type alias for convenience
using spinlock_guard_t = spinlock_guard<spinlock>;

/**
 * A more sophisticated spinlock with adaptive spinning.
 * Adjusts spin behavior based on contention.
 */
class adaptive_spinlock {
private:
    std::atomic<bool> locked_{false};
    std::atomic<int> contention_count_{0};
    static constexpr int MAX_SPIN_COUNT = 10000;
    static constexpr int MIN_SPIN_COUNT = 100;
    
public:
    adaptive_spinlock() = default;
    ~adaptive_spinlock() = default;
    
    // Delete copy constructor and assignment
    adaptive_spinlock(const adaptive_spinlock&) = delete;
    adaptive_spinlock& operator=(const adaptive_spinlock&) = delete;
    
    bool try_lock() noexcept {
        bool expected = false;
        bool success = locked_.compare_exchange_strong(expected, true,
                                                     std::memory_order_acquire,
                                                     std::memory_order_relaxed);
        if (success) {
            contention_count_.fetch_sub(1, std::memory_order_relaxed);
        }
        return success;
    }
    
    void lock() noexcept {
        int spin_count = MIN_SPIN_COUNT;
        int attempts = 0;
        
        while (!try_lock()) {
            attempts++;
            
            // Adaptive spinning based on contention
            if (attempts < spin_count) {
                // CPU pause hint
                #if defined(__x86_64__) || defined(__i386__)
                    __builtin_ia32_pause();
                #elif defined(__aarch64__)
                    __asm__ __volatile__("yield" ::: "memory");
                #endif
            } else {
                // Increase contention count and yield
                contention_count_.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::yield();
                
                // Adjust spin count based on contention
                int contention = contention_count_.load(std::memory_order_relaxed);
                spin_count = std::min(MAX_SPIN_COUNT, 
                                    MIN_SPIN_COUNT + contention * 10);
                attempts = 0;
            }
        }
    }
    
    void unlock() noexcept {
        locked_.store(false, std::memory_order_release);
    }
    
    bool is_locked() const noexcept {
        return locked_.load(std::memory_order_relaxed);
    }
    
    int get_contention_count() const noexcept {
        return contention_count_.load(std::memory_order_relaxed);
    }
};

// RAII wrapper for adaptive spinlock
using adaptive_spinlock_guard_t = spinlock_guard<adaptive_spinlock>;

#endif // SPINLOCK_H 