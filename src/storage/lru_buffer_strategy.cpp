#include "storage/lru_buffer_strategy.h"

namespace huadb {
void print_vec(std::vector<size_t> vec){
    std::cout<<"vec"<<std::endl;
    for(auto i : vec){
        std::cout<<i<<" ";
    }
    std::cout<<std::endl;
}

void print_map(std::map<size_t, int> map){
    std::cout<<"map"<<std::endl;
    for(auto i : map){
        std::cout<<i.first<<" "<<i.second<<std::endl;
    }
    // std::cout<<std::endl;
}
void LRUBufferStrategy::Access(size_t frame_no) {
  // 缓存页面访问
  // LAB 1 BEGIN
//   std::cout<<"access: "<<frame_no<<" count: "<<access_map.count(frame_no)<<std::endl;
//   print_map(access_map);
//   std::cout<<"------------------------"<<std::endl;
//   if(access_map.count(frame_no) > 0){
//     access_map[frame_no] = access_map[frame_no] + 1;
//   }else{
//     access_map[frame_no] = 1;
//   }
  auto iter = std::find(buffer_vec.begin(), buffer_vec.end(), frame_no);
  if(iter != buffer_vec.end()){
    buffer_vec.erase(iter);
  }
  buffer_vec.insert(buffer_vec.begin(), frame_no);
//   print_map(access_map);
  return;
};

size_t LRUBufferStrategy::Evict() {
  // 缓存页面淘汰，返回淘汰的页面在 buffer pool 中的下标
  // LAB 1 BEGIN
  size_t victim = buffer_vec[0];
//   int victim_cnt = access_map[victim];
//   print_vec(buffer_vec);
//   print_map(access_map);
  for(auto i:buffer_vec){
    // if(access_map[i] <= victim_cnt){
    //     victim_cnt = access_map[i];
    //     victim = i;
    // }
    victim = i;
  }
  auto iter = std::find(buffer_vec.begin(), buffer_vec.end(), victim);
  buffer_vec.erase(iter);
//   access_map[victim] = 0;
  return victim;
}
}  // namespace huadb
