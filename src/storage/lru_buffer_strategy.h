#pragma once

#include "storage/buffer_strategy.h"
#include<map>
#include<vector>
#include<iostream>
#include<algorithm>
namespace huadb {

class LRUBufferStrategy : public BufferStrategy {
 public:
  void Access(size_t frame_no) override;
  size_t Evict() override;
  std::map<size_t, int> access_map;
  std::vector<size_t> buffer_vec;
};

}  // namespace huadb
