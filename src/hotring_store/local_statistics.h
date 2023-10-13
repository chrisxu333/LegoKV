#pragma once
#include <atomic>
#include <iostream>
#include <inttypes.h>
#include <vector>
#include <assert.h>
#include "hotring_namespace.h"

namespace HOTRING_NAMESPACE {
enum StatType {
  HOTRING_LOOKUP_TIME,
  HOTRING_NEXT_TIME,
  HOTRING_RANDOM_HEAD_SHIFT,
  HOTRING_SAMPLING_HEAD_SHIFT,
  HOTRING_ENUM_MAX,
};

const std::vector<std::pair<StatType, std::string>> diagnose_mapping {
  {HOTRING_LOOKUP_TIME, "hotring.index.lookup.time"},
  {HOTRING_NEXT_TIME, "hotring.index.next.time"},
  {HOTRING_RANDOM_HEAD_SHIFT, "hotring.index.random.head.shift"},
  {HOTRING_SAMPLING_HEAD_SHIFT, "hotring.index.sampling.head.shift"},
};

class LocalStatistics {
public:
  LocalStatistics() {
    for(int i = 0; i < HOTRING_ENUM_MAX; ++i) {
      ticker[i] = 0;
    }
  }
  inline void RecordTick(StatType type, uint8_t count = 1) {
    ticker[type].fetch_add(count);
  }
  uint64_t GetTick(StatType type) {
    return ticker[type].load();
  }
private:
  std::atomic<uint64_t> ticker[HOTRING_ENUM_MAX];
};

inline void RecordTick(LocalStatistics* stat, StatType type, uint8_t count = 1) {
  if(stat)
    stat->RecordTick(type, count);
}

inline uint64_t GetTick(LocalStatistics* stat, StatType type) {
  if(!stat) return 0;
  return stat->GetTick(type);
}

inline void ShowStat(LocalStatistics* stat) {
  if(stat) {
    printf("========== HotRing Statistics ==========\n");
    for (const auto& t : diagnose_mapping) {
      assert(t.first < HOTRING_ENUM_MAX);
      printf("%s: \t%" PRIu64 "\n", t.second.c_str(), stat->GetTick(t.first));
    }
    printf("========================================\n");
  }
}
}