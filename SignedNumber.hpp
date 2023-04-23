#pragma once

#include <bitset>
#include <cmath>
#include <cstdint>

template <int64_t SIZE> const int64_t signed_number(const uint64_t val) {
  int64_t number = 0;
  for (int i = 0; i < SIZE - 1; i++)
    number += ((val >> i) & 1) * (1 << i);
  return number - (val >> (SIZE - 1)) * (1 << (SIZE - 1));
}