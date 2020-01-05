#include "test-runner.hpp"

#include "gtest/gtest.h"

TEST(Gekkio, tim00) {
  runTest(GEKKIO, "acceptance/timer/tim00.gb");
}

TEST(Gekkio, tim01) {
  runTest(GEKKIO, "acceptance/timer/tim01.gb");
}

TEST(Gekkio, tim10) {
  runTest(GEKKIO, "acceptance/timer/tim10.gb");
}

TEST(Gekkio, tim11) {
  runTest(GEKKIO, "acceptance/timer/tim11.gb");
}

TEST(Gekkio, div_write) {
  runTest(GEKKIO, "acceptance/timer/div_write.gb");
}

TEST(Gekkio, rom_4Mb) {
  runTest(GEKKIO, "emulator-only/mbc1/rom_4Mb.gb");
}

TEST(Gekkio, oam_dma_basic) {
  runTest(GEKKIO, "acceptance/oam_dma/basic.gb");
}

TEST(Gekkio, mem_oam) {
  runTest(GEKKIO, "acceptance/bits/mem_oam.gb");
}

TEST(Gekkio, reg_f) {
  runTest(GEKKIO, "acceptance/bits/reg_f.gb");
}

TEST(Gekkio, daa) {
  runTest(GEKKIO, "acceptance/instr/daa.gb");
}
