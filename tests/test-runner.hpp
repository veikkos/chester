#pragma once

enum RomType {
  BLARGG,
  GEKKIO
};

void runTest(RomType romType, const char* romPath);
