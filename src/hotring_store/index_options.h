#pragma once
#include "hotring_namespace.h"
#include <inttypes.h>
namespace HOTRING_NAMESPACE {
struct IndexOptions {
  // number of hash buckets. Default is 10.
  uint64_t bucket_size = 10;
  // use random shift policy for head pointer movement
  bool use_random_shift = false;
  // use statistical sampling shift policy for head pointer movement
  bool use_sampling_shift = false;
  // request count threshold for random shift policy
  uint32_t R = 5;
};
}