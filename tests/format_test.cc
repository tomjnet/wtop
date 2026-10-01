#include "core/format.h"

#include <limits>
#include <string>

#include "gtest/gtest.h"

namespace wtop {
namespace {

constexpr double kKiB = 1024.0;
constexpr double kMiB = kKiB * 1024.0;
constexpr double kGiB = kMiB * 1024.0;

TEST(Format, Bytes) {
  EXPECT_EQ(FormatBytes(0), std::string("0 B"));
  EXPECT_EQ(FormatBytes(512), std::string("512 B"));
  EXPECT_EQ(FormatBytes(256 * kKiB), std::string("256 KB"));
  EXPECT_EQ(FormatBytes(1 * kMiB), std::string("1.0 MB"));
  EXPECT_EQ(FormatBytes(834 * kMiB), std::string("834 MB"));
  EXPECT_EQ(FormatBytes(15.2 * kGiB), std::string("15.2 GB"));
  EXPECT_EQ(FormatBytes(932 * kGiB), std::string("932 GB"));
  EXPECT_EQ(FormatBytes(1000 * kGiB), std::string("1.0 TB"));
}

TEST(Format, Rates) {
  EXPECT_EQ(FormatBytesPerSec(0), std::string("0 KB/s"));
  EXPECT_EQ(FormatBytesPerSec(208 * kKiB), std::string("208 KB/s"));
  EXPECT_EQ(FormatBytesPerSec(60 * kMiB), std::string("60.0 MB/s"));
  EXPECT_EQ(FormatBitsPerSec(0), std::string("0 Kbps"));
  EXPECT_EQ(FormatBitsPerSec(800e3), std::string("800 Kbps"));
  EXPECT_EQ(FormatBitsPerSec(1.2e6), std::string("1.2 Mbps"));
}

TEST(Format, Duration) {
  EXPECT_EQ(FormatDuration(0), std::string("0:00:00:00"));
  const std::uint64_t uptime = 6 * 86400 + 17 * 3600 + 47 * 60 + 15;
  EXPECT_EQ(FormatDuration(uptime), std::string("6:17:47:15"));
}

TEST(Format, FrequencyPercentAndPairs) {
  EXPECT_EQ(FormatFrequency(3700), std::string("3.70 GHz"));
  EXPECT_EQ(FormatFrequency(800), std::string("800 MHz"));
  EXPECT_EQ(FormatFrequency(0), std::string("N/A"));
  EXPECT_EQ(FormatPercent(6.4), std::string("6%"));
  EXPECT_EQ(FormatPercent(std::numeric_limits<double>::quiet_NaN()),
            std::string("N/A"));
  EXPECT_EQ(FormatBytesPair(0.5 * kGiB, 15.9 * kGiB),
            std::string("0.5/15.9 GB"));
  EXPECT_EQ(FormatBytesPair(16.4 * kGiB, 38.5 * kGiB),
            std::string("16.4/38.5 GB"));
}

TEST(Format, NiceCeiling) {
  EXPECT_EQ(NiceCeiling(0), 1.0);
  EXPECT_EQ(NiceCeiling(1), 1.0);
  EXPECT_EQ(NiceCeiling(1.5), 2.0);
  EXPECT_EQ(NiceCeiling(3), 5.0);
  EXPECT_EQ(NiceCeiling(100), 100.0);
  EXPECT_EQ(NiceCeiling(101), 200.0);
  EXPECT_EQ(NiceCeiling(600), 1000.0);
}

}  // namespace
}  // namespace wtop
