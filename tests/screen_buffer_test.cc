#include "terminal/screen_buffer.h"

#include <string>

#include "gtest/gtest.h"
#include "ui/widgets.h"

namespace wtop {
namespace {

TEST(ScreenBuffer, Utf8RoundTrip) {
  const std::string text = "CPU ⣿ 43 °C";
  const std::u32string cps = DecodeUtf8(text);
  EXPECT_EQ(cps.size(), 11u);
  std::string back;
  for (char32_t cp : cps) AppendUtf8(cp, back);
  EXPECT_EQ(back, text);
  EXPECT_EQ(DisplayWidth(text), 11);
}

TEST(ScreenBuffer, DrawTextClipsAndPlainText) {
  ScreenBuffer buffer(8, 2);
  EXPECT_EQ(buffer.DrawText(0, 0, "Memory usage", Style{}), 8);
  buffer.DrawText(6, 1, "abc", Style{});
  EXPECT_EQ(buffer.ToPlainText(), "Memory u\n      ab\n");
}

TEST(ScreenBuffer, RenderOnlyEmitsChanges) {
  ScreenBuffer previous(10, 3);
  ScreenBuffer next(10, 3);
  EXPECT_TRUE(next.Render(previous, false).empty());
  next.Set(4, 1, U'x', Style{});
  const std::string diff = next.Render(previous, false);
  EXPECT_NE(diff.find("\x1b[2;5H"), std::string::npos);
  EXPECT_NE(diff.find('x'), std::string::npos);
}

TEST(Widgets, GraphFillsFromTheBottom) {
  History history;
  for (int i = 0; i < 60; ++i) history.Push(100.0);
  ScreenBuffer buffer(4, 2);
  DrawGraph(buffer, Rect{0, 0, 4, 2}, {GraphSeries{&history, Color{}, true}},
            GraphOptions{.max_value = 100.0, .grid = false});
  EXPECT_TRUE(buffer.At(3, 0).ch == U'⣿');
  EXPECT_TRUE(buffer.At(0, 1).ch == U'⣿');
}

TEST(Widgets, Truncate) {
  EXPECT_EQ(Truncate("GeForce GTX 1650", 7), "GeForc…");
  EXPECT_EQ(Truncate("CPU", 7), "CPU");
}

}  // namespace
}  // namespace wtop
