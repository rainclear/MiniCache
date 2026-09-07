#ifndef MINICACHE_OBJECT_POOL_HPP
#define MINICACHE_OBJECT_POOL_HPP

#include <memory>
#include <queue>
#include <mutex>
#include <functional>
#include <concepts>
#include <cstddef>

namespace minicache {

/**
 * @brief C++20 Concept checking if type T provides a void reset() member function.
 */
template <typename T>
concept Resettable = requires(T t) {
    { t.reset() } -> std::same_as<void>;
};

/**
 * @brief Thread-safe generic ObjectPool for recycling heavy objects or buffers.
 * Uses a custom deleter with std::unique_ptr for automatic, transparent object recycling.
 */
template <typename T>
class ObjectPool : public std::enable_shared_from_this<ObjectPool<T>> {
public:
    // Custom smart pointer alias that recycles objects back to the pool upon destruction
    using PooledPtr = std::unique_ptr<T, std::function<void(T*)>>;

    explicit ObjectPool(std::size_t initial_capacity = 10) {
        for (std::size_t i = 0; i < initial_capacity; ++i) {
            pool_.push(std::make_unique<T>());
        }
        total_created_ = initial_capacity;
    }

    ~ObjectPool() = default;

    // Non-copyable and non-movable
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    /**
     * @brief Acquires an object from the pool (or allocates a new one if empty).
     * @return PooledPtr Custom unique_ptr with auto-recycling deleter.
     */
    [[nodiscard]] PooledPtr acquire() {
        std::unique_ptr<T> instance;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!pool_.empty()) {
                instance = std::move(pool_.front());
                pool_.pop();
            } else {
                instance = std::make_unique<T>();
                ++total_created_;
            }
        }

        // Use a weak_ptr to prevent circular ownership with the ObjectPool instance
        std::weak_ptr<ObjectPool<T>> weak_pool = this->shared_from_this();

        // Construct unique_ptr with custom deleter
        return PooledPtr(instance.release(), [weak_pool](T* ptr) {
            if (!ptr) return;

            // Automatically reset object state if type T satisfies the Resettable concept
            if constexpr (Resettable<T>) {
                ptr->reset();
            }

            if (auto pool = weak_pool.lock()) {
                pool->recycle(std::unique_ptr<T>(ptr));
            } else {
                delete ptr; // Pool instance is no longer active, free directly
            }
        });
    }

    /**
     * @brief Returns the number of currently available objects sitting in the pool.
     */
    [[nodiscard]] std::size_t available_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

    /**
     * @brief Returns the total count of objects created across the lifetime of this pool.
     */
    [[nodiscard]] std::size_t total_created_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return total_created_;
    }

private:
    void recycle(std::unique_ptr<T> instance) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push(std::move(instance));
    }

    std::queue<std::unique_ptr<T>> pool_;
    mutable std::mutex mutex_;
    std::size_t total_created_{0};
};

} // namespace minicache

#endif // MINICACHE_OBJECT_POOL_HPP