#pragma once

#include <string>
#include<iostream>
#include "common/types.h"
#include "log/log_manager.h"
#include "storage/page.h"
#include "table/record.h"

namespace huadb {

class FSMPage {
 public:
  explicit FSMPage(std::shared_ptr<Page> page);

  // 页面初始化
  void Init(int is_root, int is_leaf);

  void SetLeaf(int is_leaf);

  void SetRoot(int is_root);

  int GetLeaf();

  int GetRoot();

  // 插入页面
  FSMReturn InsertPage(pageid_t pageid, db_size_t page_size, int level_now);
  // 删除记录
//   FSMReturn DeletePage(pageid_t pageid, int level_now);
  // 更新页面大小
  FSMReturn UpdatePage(pageid_t pageid, db_size_t new_size, int level_now);
  // 向上更新
  void UpdateNodes(int node);
  // 从根开始寻找 id 为 node 的页面大小位置
  int SearchFromRoot(int* tmp, int cnt);
  // 寻找有空闲空间的页面
  FSMReturn SearchPage(int need_size);
  // 从十进制转化为二进制
  int ChangeToBinary(pageid_t num, int* tmp);
  // 获取第 i 个孩子节点的 pageid
  pageid_t GetChildId(int i);

  db_size_t GetMax();

  void SetChildren(int i, pageid_t children);



  void ToString() const;

 private:
  std::shared_ptr<Page> page_;
  char *page_data_;
  FSMPageId* fp_nodes;
  int* length;
  int* is_root_;
  int* is_leaf_;
  pageid_t* children_ids;
};

}  // namespace huadb