#include "test-runner.hpp"

#include "gtest/gtest.h"

TEST(Blargg, cpu_instrs) {
  runTest(BLARGG, "cpu_instrs/cpu_instrs.gb");
}

TEST(Blargg, instr_timing) {
  runTest(BLARGG, "instr_timing/instr_timing.gb");
}

// Disabled due to missing serial output in test ROM
TEST(Blargg, DISABLED_3_non_causes) {
  runTest(BLARGG, "oam_bug/rom_singles/3-non_causes.gb");
}

// Disabled due to missing serial output in test ROM
TEST(Blargg, DISABLED_6_timing_no_bug) {
  runTest(BLARGG, "oam_bug/rom_singles/6-timing_no_bug.gb");
}
