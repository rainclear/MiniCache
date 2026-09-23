#ifndef MINICACHE_NET_BUFFER_HPP
#define MINICACHE_NET_BUFFER_HPP

#include <array>
#include <cstddef>
#include <algorithm>

namespace minicache::net {

class NetworkBuffer {
public:
    static constexpr std::size_t kCapacity = 4096;

    NetworkBuffer() {
        data_.fill(0);
    }

    void reset() {
        std::fill(data_.begin(), data_.begin() + size_, 0);
        size_ = 0;
    }

    [[nodiscard]] char* data() noexcept { return data_.data(); }
    [[nodiscard]] const char* data() const noexcept { return data_.data(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return kCapacity; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    void set_size(std::size_t s) noexcept { size_ = std::min(s, kCapacity); }

private:
    std::array<char, kCapacity> data_;
    std::size_t size_{0};
};

} // namespace minicache::net

#endif // MINICACHE_NET_BUFFER_HPP