#ifndef WTOP_SRC_CORE_RING_BUFFER_H_
#define WTOP_SRC_CORE_RING_BUFFER_H_

#include <algorithm>
#include <array>
#include <cstddef>

namespace wtop {

// Fixed-capacity circular buffer. Once full, pushing drops the oldest value.
// Index 0 is the oldest element and index size() - 1 the newest.
template <typename T, std::size_t N>
class RingBuffer {
 public:
  static_assert(N > 0, "RingBuffer capacity must be positive");

  void Push(const T& value) {
    data_[(start_ + size_) % N] = value;
    if (size_ < N) {
      ++size_;
    } else {
      start_ = (start_ + 1) % N;
    }
  }

  void Clear() {
    start_ = 0;
    size_ = 0;
  }

  static constexpr std::size_t capacity() { return N; }
  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  const T& operator[](std::size_t i) const { return data_[(start_ + i) % N]; }
  const T& back() const { return (*this)[size_ - 1]; }

  // Largest stored value, or a value-initialized T when empty.
  T Max() const {
    T result{};
    for (std::size_t i = 0; i < size_; ++i) {
      result = std::max(result, (*this)[i]);
    }
    return result;
  }

 private:
  std::array<T, N> data_{};
  std::size_t start_ = 0;
  std::size_t size_ = 0;
};

// Every graph in wtop shows the last 60 samples, like Task Manager.
inline constexpr std::size_t kHistorySize = 60;
using History = RingBuffer<double, kHistorySize>;

}  // namespace wtop

#endif  // WTOP_SRC_CORE_RING_BUFFER_H_
