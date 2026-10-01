#include "core/ring_buffer.h"

#include "gtest/gtest.h"

namespace wtop {
namespace {

TEST(RingBuffer, KeepsNewestValuesInOrder) {
  RingBuffer<int, 3> buffer;
  EXPECT_TRUE(buffer.empty());
  for (int i = 1; i <= 5; ++i) buffer.Push(i);
  EXPECT_EQ(buffer.size(), 3u);
  EXPECT_EQ(buffer[0], 3);
  EXPECT_EQ(buffer[1], 4);
  EXPECT_EQ(buffer[2], 5);
  EXPECT_EQ(buffer.back(), 5);
  EXPECT_EQ(buffer.Max(), 5);
  buffer.Clear();
  EXPECT_TRUE(buffer.empty());
  EXPECT_EQ(buffer.Max(), 0);
}

}  // namespace
}  // namespace wtop
