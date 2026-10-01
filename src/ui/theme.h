#ifndef WTOP_SRC_UI_THEME_H_
#define WTOP_SRC_UI_THEME_H_

#include "terminal/screen_buffer.h"

namespace wtop::theme {

// Resource colors, modelled on Task Manager's performance page.
inline constexpr Color kCpu = Color::Rgb(41, 170, 214);
inline constexpr Color kMemory = Color::Rgb(112, 152, 240);
inline constexpr Color kDisk = Color::Rgb(118, 190, 64);
inline constexpr Color kNetwork = Color::Rgb(232, 92, 150);
inline constexpr Color kGpu = Color::Rgb(168, 110, 238);

// Chrome.
inline constexpr Color kBlack = Color::Rgb(16, 16, 20);
inline constexpr Color kText = Color::Rgb(228, 228, 234);
inline constexpr Color kLabel = Color::Rgb(146, 148, 160);
inline constexpr Color kBorder = Color::Rgb(84, 88, 100);
inline constexpr Color kGrid = Color::Rgb(52, 56, 66);
inline constexpr Color kPanel = Color::Rgb(28, 30, 36);
inline constexpr Color kSelected = Color::Rgb(46, 50, 62);
inline constexpr Color kTabInactive = Color::Rgb(50, 53, 63);
inline constexpr Color kTabInactiveText = Color::Rgb(182, 184, 196);
inline constexpr Color kStatus = Color::Rgb(84, 168, 84);
inline constexpr Color kPrefix = Color::Rgb(236, 196, 64);
inline constexpr Color kFunctionBar = Color::Rgb(56, 170, 180);

inline constexpr Style kLabelStyle{kLabel, Color::Default(), kNormal};
inline constexpr Style kTextStyle{kText, Color::Default(), kNormal};
inline constexpr Style kValueStyle{kText, Color::Default(), kBold};
inline constexpr Style kBorderStyle{kBorder, Color::Default(), kNormal};

}  // namespace wtop::theme

#endif  // WTOP_SRC_UI_THEME_H_
