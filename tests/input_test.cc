#include "terminal/input.h"

#include "gtest/gtest.h"

namespace wtop {
namespace {

TEST(Input, DecodesTmuxPrefixArrowsAndControlKeys) {
  const auto keys = DecodeKeys("\x02n\x1b[A\x1b[Z\r\x1b");
  ASSERT_EQ(keys.size(), 6u);
  EXPECT_TRUE(keys[0].IsChar(kCtrlB));
  EXPECT_TRUE(keys[1].IsChar('n'));
  EXPECT_EQ(keys[2].code, KeyCode::kUp);
  EXPECT_EQ(keys[3].code, KeyCode::kBackTab);
  EXPECT_EQ(keys[4].code, KeyCode::kEnter);
  EXPECT_EQ(keys[5].code, KeyCode::kEscape);
}

// F1 differs between terminals: xterm (SS3), vt220/rxvt ("11~") and the
// Linux console ("[[A"). All must decode to the same key.
TEST(Input, DecodesF1FromEveryTerminalFlavour) {
  for (const char* sequence : {"\x1bOP", "\x1b[11~", "\x1b[[A", "\x1b[1;2P"}) {
    const auto keys = DecodeKeys(sequence);
    ASSERT_EQ(keys.size(), 1u) << "sequence: " << sequence + 1;
    EXPECT_EQ(keys[0].code, KeyCode::kF1) << "sequence: " << sequence + 1;
  }
}

TEST(Input, DecodesF10AndOtherFunctionKeys) {
  const auto keys = DecodeKeys("\x1b[21~\x1b[15~\x1b[17~\x1b[24~\x1bOS");
  ASSERT_EQ(keys.size(), 5u);
  EXPECT_EQ(keys[0].code, KeyCode::kF10);
  EXPECT_EQ(keys[1].code, KeyCode::kF5);
  EXPECT_EQ(keys[2].code, KeyCode::kF6);
  EXPECT_EQ(keys[3].code, KeyCode::kF12);
  EXPECT_EQ(keys[4].code, KeyCode::kF4);
}

TEST(Input, NavigationKeysStillDecode) {
  const auto keys = DecodeKeys("\x1b[5~\x1b[6~\x1b[1~\x1b[4~");
  ASSERT_EQ(keys.size(), 4u);
  EXPECT_EQ(keys[0].code, KeyCode::kPageUp);
  EXPECT_EQ(keys[1].code, KeyCode::kPageDown);
  EXPECT_EQ(keys[2].code, KeyCode::kHome);
  EXPECT_EQ(keys[3].code, KeyCode::kEnd);
}

}  // namespace
}  // namespace wtop
