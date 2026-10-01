#include <cmath>
#include <string>

#include "collectors/proc_parsers.h"
#include "core/pci_ids.h"
#include "gtest/gtest.h"

namespace wtop {
namespace {

TEST(Parsers, ProcStat) {
  const auto cpus = ParseProcStat(
      "cpu  100 0 50 800 50 0 0 0 0 0\n"
      "cpu0 60 0 20 400 20 0 0 0 0 0\n"
      "cpu1 40 0 30 400 30 0 0 0 0 0\n"
      "intr 12345\n"
      "ctxt 999\n");
  EXPECT_EQ(cpus.size(), 3u);
  EXPECT_EQ(cpus[0].user, 100u);
  EXPECT_EQ(cpus[0].Idle(), 850u);
  EXPECT_EQ(cpus[0].Total(), 1000u);
  EXPECT_EQ(cpus[2].system, 30u);
}

TEST(Parsers, CpuUtilization) {
  CpuTimes before;
  before.user = 100;
  before.idle = 900;
  CpuTimes after = before;
  after.user += 25;
  after.idle += 75;
  EXPECT_NEAR(CpuUtilization(before, after), 25.0, 1e-9);
  EXPECT_NEAR(CpuUtilization(after, after), 0.0, 1e-9);
  EXPECT_NEAR(CpuUtilization(after, before), 0.0, 1e-9);  // Counter reset.
}

TEST(Parsers, Meminfo) {
  const auto info = ParseMeminfo(
      "MemTotal:       32768000 kB\n"
      "MemFree:         1000 kB\n"
      "HugePages_Total:       0\n");
  EXPECT_EQ(info.at("MemTotal"), 32768000ull * 1024);
  EXPECT_EQ(info.at("MemFree"), 1000ull * 1024);
  EXPECT_EQ(info.at("HugePages_Total"), 0ull);
}

TEST(Parsers, Diskstats) {
  const auto disks = ParseDiskstats(
      "   8       0 sda 100 5 2000 30 200 10 4000 60 0 80 90 0 0 0 0\n"
      "   8       1 sda1 50 0 1000 10 20 0 400 6 0 12 16\n"
      "bogus line\n");
  EXPECT_EQ(disks.size(), 2u);
  EXPECT_EQ(disks[0].name, std::string("sda"));
  EXPECT_EQ(disks[0].reads, 100u);
  EXPECT_EQ(disks[0].sectors_read, 2000u);
  EXPECT_EQ(disks[0].read_ms, 30u);
  EXPECT_EQ(disks[0].writes, 200u);
  EXPECT_EQ(disks[0].sectors_written, 4000u);
  EXPECT_EQ(disks[0].write_ms, 60u);
  EXPECT_EQ(disks[0].io_ms, 80u);
}

TEST(Parsers, NetDev) {
  const auto adapters = ParseNetDev(
      "Inter-|   Receive                            |  Transmit\n"
      " face |bytes    packets errs drop fifo frame compressed multicast|"
      "bytes    packets errs drop fifo colls carrier compressed\n"
      "    lo:   27942     212    0    0    0     0          0         0 "
      "   27942     212    0    0    0     0       0          0\n"
      "  eth0: 9510694    1324    0    0    0     0          0         0 "
      "  162331    1623    0    0    0     0       0          0\n");
  EXPECT_EQ(adapters.size(), 2u);
  EXPECT_EQ(adapters[1].name, std::string("eth0"));
  EXPECT_EQ(adapters[1].rx_bytes, 9510694u);
  EXPECT_EQ(adapters[1].tx_bytes, 162331u);
}

TEST(Parsers, CpuListAndCacheSize) {
  const auto cpus = ParseCpuList("0-2,5,7-8\n");
  EXPECT_EQ(cpus.size(), 6u);
  EXPECT_EQ(cpus.front(), 0);
  EXPECT_EQ(cpus.back(), 8);
  EXPECT_EQ(ParseCacheSize("32K").value_or(0), 32ull * 1024);
  EXPECT_EQ(ParseCacheSize("8192K").value_or(0), 8ull * 1024 * 1024);
  EXPECT_EQ(ParseCacheSize("1M").value_or(0), 1ull * 1024 * 1024);
  EXPECT_TRUE(!ParseCacheSize("garbage").has_value());
}

TEST(Parsers, ModelFrequency) {
  EXPECT_NEAR(ParseModelFrequencyMhz("Intel(R) Core(TM) i5-9300H CPU @ 2.40GHz")
                  .value_or(0),
              2400.0, 1e-6);
  EXPECT_TRUE(!ParseModelFrequencyMhz("AMD Ryzen 7 5800X 8-Core Processor")
                   .has_value());
}

TEST(Parsers, Cpuinfo) {
  const auto entries = ParseCpuinfo(
      "processor\t: 0\nmodel name\t: Test CPU @ 2.40GHz\ncpu MHz\t\t: "
      "2400.000\nphysical id\t: 0\ncore id\t\t: 0\nflags\t\t: fpu vmx\n\n"
      "processor\t: 1\nmodel name\t: Test CPU @ 2.40GHz\ncpu MHz\t\t: "
      "3100.5\nphysical id\t: 0\ncore id\t\t: 1\nflags\t\t: fpu vmx\n");
  EXPECT_EQ(entries.size(), 2u);
  EXPECT_EQ(entries[0].model_name, std::string("Test CPU @ 2.40GHz"));
  EXPECT_NEAR(entries[1].mhz, 3100.5, 1e-9);
  EXPECT_EQ(entries[1].core_id, 1);
  EXPECT_EQ(entries[0].flags, std::string("fpu vmx"));
}

TEST(Parsers, NvidiaSmi) {
  const auto gpus = ParseNvidiaSmiCsv(
      "0, NVIDIA GeForce GTX 1650, 616.92, 00000000:01:00.0, 7, 3, 0, 0, "
      "13, 4096, 42, 3.19, [N/A], 300, 2100, [N/A]\n");
  EXPECT_EQ(gpus.size(), 1u);
  EXPECT_EQ(gpus[0].name, std::string("NVIDIA GeForce GTX 1650"));
  EXPECT_EQ(gpus[0].driver_version, std::string("616.92"));
  EXPECT_NEAR(gpus[0].utilization, 7.0, 1e-9);
  EXPECT_NEAR(gpus[0].memory_total_mib, 4096.0, 1e-9);
  EXPECT_NEAR(gpus[0].temperature_c, 42.0, 1e-9);
  EXPECT_TRUE(std::isnan(gpus[0].power_limit_w));
  EXPECT_TRUE(std::isnan(gpus[0].fan_percent));
}

TEST(Parsers, PciIds) {
  const std::string db =
      "# comment\n"
      "10de  NVIDIA Corporation\n"
      "\t1f91  TU117M [GeForce GTX 1650 Mobile / Max-Q]\n"
      "\t\t1043 1f91  subsystem entry\n"
      "8086  Intel Corporation\n"
      "\t2723  Wi-Fi 6 AX200\n"
      "C 00  Unclassified device\n";
  const PciName wifi = LookupPciName(db, 0x8086, 0x2723);
  EXPECT_EQ(wifi.vendor, std::string("Intel Corporation"));
  EXPECT_EQ(wifi.device, std::string("Wi-Fi 6 AX200"));
  const PciName gpu = LookupPciName(db, 0x10de, 0x1f91);
  EXPECT_EQ(gpu.device,
            std::string("TU117M [GeForce GTX 1650 Mobile / Max-Q]"));
  EXPECT_TRUE(LookupPciName(db, 0x10de, 0x1043).device.empty());
}

}  // namespace
}  // namespace wtop
