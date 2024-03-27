
#include "table/table.h"

#include "table/table_page.h"
#include "table/fsm_page.h"

namespace huadb {

Table::Table(BufferPool &buffer_pool, LogManager &log_manager, oid_t oid, oid_t db_oid, ColumnList column_list,
             bool new_table, bool is_empty)
    : buffer_pool_(buffer_pool),
      log_manager_(log_manager),
      oid_(oid),
      db_oid_(db_oid),
      column_list_(std::move(column_list)) {
  if (new_table || is_empty) {
    first_page_id_ = NULL_PAGE_ID;
    fsm_root_id_ = NULL_PAGE_ID;
    max_page_id_ = 0;
  } else {
    first_page_id_ = 0;
    fsm_root_id_ = 1;
    auto page = buffer_pool_.GetPage(db_oid_, oid_, 0);
    auto table_page = std::make_shared<TablePage>(page);
    pageid_t next_page_id = table_page->GetNextPageId();
    max_page_id_ = 0;
    while(next_page_id != NULL_PAGE_ID){
        auto next_page = buffer_pool.GetPage(db_oid, oid_, next_page_id);
        auto next_table_page = std::make_shared<TablePage>(next_page);
        next_page_id = next_table_page->GetNextPageId();
        max_page_id_++;
    }
    max_page_id_++;
  }
  
}

void Table::UpdateFSM(pageid_t pageid, std::map<pageid_t, db_size_t> page_size_map){
    auto fsm = buffer_pool_.GetPage(db_oid_,oid_, pageid);
    auto fsm_page = std::make_shared<FSMPage>(fsm);
    int page_children[8];
    for(int i = 0; i < 8; i++){
        page_children[i] = fsm_page->GetChildId(i);
        if(page_children[i] % 2 == 1 && page_children[i] != NULL_PAGE_ID){
            UpdateFSM(page_children[i], page_size_map);
            auto child = buffer_pool_.GetPage(db_oid_, oid_, page_children[i]);
            auto child_page = std::make_shared<FSMPage>(child);
            db_size_t child_max = child_page->GetMax();
            fsm_page->UpdatePage(i, child_max, 0);
        }else{
            auto it = page_size_map.find(page_children[i]);
            if(it != page_size_map.end()){
                // std::cout<<i<<"    "<<page_children[i]<<"   "<<page_size_map[page_children[i]]<<std::endl;
                fsm_page->UpdatePage(page_children[i] / 2,page_size_map[page_children[i]],0);
            }
        }
    }
    bool flag = true;
}

void Table::Vacuum(){
    pageid_t page_id = first_page_id_;
    pageid_t last_page_id;
    std::map<pageid_t, db_size_t> page_size_map;
    while(page_id != NULL_PAGE_ID){
        auto page = buffer_pool_.GetPage(db_oid_, oid_, page_id);
        auto table_page = std::make_unique<TablePage>(page);
        bool is_empty = table_page->PageVacuum(column_list_);
        page_size_map[page_id] = table_page->GetFreeSpaceSize();
        if(is_empty){
            page_size_map[page_id] = 0;
            if(page_id != first_page_id_){
                auto next_page_id = table_page->GetNextPageId();
                auto last_page = buffer_pool_.GetPage(db_oid_, oid_, last_page_id);
                auto last_table_page = std::make_unique<TablePage>(last_page);
                last_table_page->SetNextPageId(table_page->GetNextPageId());
            }else{
                first_page_id_ = table_page->GetNextPageId();
            }
        }
        last_page_id = page_id;
        page_id = table_page->GetNextPageId();
    }
    auto fsm_root_page = buffer_pool_.GetPage(db_oid_,oid_, fsm_root_id_);
    auto root_fsm = std::make_shared<FSMPage>(fsm_root_page);
    int page_children[8];
    for(int i = 0; i < 8; i++){
        // std::cout<<"page_children "<<i<<" "<<page_children[i]<<std::endl;
        page_children[i] = root_fsm->GetChildId(i);
    }
    bool flag = true;
    UpdateFSM(fsm_root_id_, page_size_map);
    // root_fsm->ToString();
}

Rid Table::InsertRecord(std::shared_ptr<Record> record, xid_t xid, cid_t cid, bool write_log) {
  if (record->GetSize() > MAX_RECORD_SIZE) {
    throw DbException("Record size too large: " + std::to_string(record->GetSize()));
  }
    // std::cout<<"insert record"<<std::endl;
  // throw DbException("Record size too large: " + std::to_string(record->GetSize()));
  // 当 write_log 参数为 true 时开启写日志功能
  // 在插入记录时增加写 InsertLog 过程
  // 在创建新的页面时增加写 NewPageLog 过程
  // 设置页面的 page lsn
  // LAB 2 BEGIN

  // 使用 buffer_pool_ 获取页面
  // 使用 TablePage 类操作记录页面
  // 遍历表的页面，判断页面是否有足够的空间插入记录，如果没有则通过 buffer_pool_ 创建新页面
  // 如果 first_page_i_ 为 NULL_PAGE_ID，说明表还没有页面，需要创建新页面
  // 创建新页面时需设置前一个页面的 next_page_id，并将新页面初始化
  // 找到空间足够的页面后，通过 TablePage 插入记录
  // 返回插入记录的 rid
  // LAB 1 BEGIN
  pageid_t insert_page_id;
  slotid_t insert_slot_id;
    //  std::cout<<first_page_id_<<std::endl;
  if (first_page_id_ == NULL_PAGE_ID) {
    // std::cout<<"first"<<std::endl;
    auto page = buffer_pool_.NewPage(db_oid_, oid_, 0);
    first_page_id_ = 0;
    max_page_id_++;
    auto table_page = std::make_shared<TablePage>(page);
    table_page->Init();
    insert_slot_id = table_page->InsertRecord(record, xid, cid);
    db_size_t free_space = table_page->GetFreeSpaceSize();
    insert_page_id = 0;
    fsm_root_id_ = 1;
    auto page_for_fsm = buffer_pool_.NewPage(db_oid_, oid_, fsm_root_id_);
    auto fsm_page = std::make_shared<FSMPage>(page_for_fsm);
    fsm_min_page_id_ = 0;
    fsm_page->Init(1, 1);
    fsm_page->InsertPage(first_page_id_, free_space, 0);
  } else {
    // std::cout<<"second"<<std::endl;
    auto page = buffer_pool_.GetPage(db_oid_, oid_, fsm_root_id_);
    auto fsm_page = std::make_shared<FSMPage>(page);
    // fsm_page->ToString();
    int is_leaf = fsm_page->GetLeaf();
    bool flag = true;
    std::vector<std::shared_ptr<FSMPage>> fsm_vec;
    fsm_vec.push_back(fsm_page);
    FSMReturn fsm_return_search = fsm_page->SearchPage(record->GetSize());
    while (is_leaf != 1) {
      pageid_t page_id_now = fsm_return_search.pageid_;
      if (page_id_now == NULL_PAGE_ID) {
        flag = false;
        break;
      } else {
        auto next_page = buffer_pool_.GetPage(db_oid_, oid_, page_id_now);
        auto next_fsm_page = std::make_shared<FSMPage>(next_page);
        fsm_vec.push_back(next_fsm_page);
        fsm_return_search = next_fsm_page->SearchPage(record->GetSize());
        is_leaf = next_fsm_page->GetLeaf();
      }
    }
    db_size_t record_size = record->GetSize();
    // FSMReturn fsm_return = fsm_page->SearchPage(record_size);
    // std::cout<<"fsm return: "<<fsm_return_search.pageid_<<std::endl;
    pageid_t page_id_now = fsm_return_search.pageid_;
    if (page_id_now == NULL_PAGE_ID) {
      flag = false;
    }
    // std::cout<<"fsm root"<<fsm_root_id_<<std::endl;
    auto fsm_root = buffer_pool_.GetPage(db_oid_, oid_, fsm_root_id_);
    // std::cout<<"get fsm root"<<fsm_root_id_<<std::endl;
    auto fsm_root_page = std::make_shared<FSMPage>(fsm_root);
    if (flag) {
      // 存在对应的空闲页面与，直接找到已有的page插入即可
    //   std::cout<<"flag true: "<<page_id_now<<std::endl;
      auto page_insert = buffer_pool_.GetPage(db_oid_, oid_, page_id_now);
      auto insert_table_page = std::make_shared<TablePage>(page_insert);
      insert_page_id = page_id_now;
      insert_slot_id = insert_table_page->InsertRecord(record, xid, cid);
      int level = 0;
      db_size_t max_size_update = insert_table_page->GetFreeSpaceSize();
      // 更新空闲数组（从下往上）
    //   std::cout<<"update fsm(down->up)"<<std::endl;
      for (auto it = fsm_vec.rbegin(); it != fsm_vec.rend(); it++) {
        // (*it)->ToString();
        (*it)->UpdatePage(page_id_now / 2, max_size_update, level);
        max_size_update = (*it)->GetMax();
        level++;
      }
    //   std::cout<<"flag true finish"<<std::endl;
    } else {
      // 已有的页面没有足够的空间，新开一个页面
    //   std::cout<<"max page id: "<<max_page_id_<<std::endl;
      auto last_page = buffer_pool_.GetPage(db_oid_,oid_,(max_page_id_ - 1)*2);
      auto last_table_page = std::make_shared<TablePage>(last_page);
      last_table_page->SetNextPageId(max_page_id_*2);
      pageid_t last_id = last_table_page->GetNextPageId();
    //   std::cout<<last_id<<std::endl;
      auto new_page = buffer_pool_.NewPage(db_oid_, oid_, max_page_id_*2);
      max_page_id_++;
      auto new_table_page = std::make_shared<TablePage>(new_page);
      new_table_page->Init();
      insert_page_id = (max_page_id_ - 1)*2;
      insert_slot_id = new_table_page->InsertRecord(record, xid, cid);
      db_size_t insert_page_free_size = new_table_page->GetFreeSpaceSize();
      int tmp[33];
      int cnt = fsm_page->ChangeToBinary(insert_page_id / 2, tmp);
      int level = 0;
      std::vector<std::shared_ptr<FSMPage>> insert_fsm_vec;
      int leaf = fsm_page->GetLeaf();
      int root = fsm_page->GetRoot();
      int num_now = 0;
      int count_four = 0;
      for (int i = 3; i >= 0; i--) {
        num_now *= 2;
        num_now += tmp[i];
        count_four++;
        if (count_four >= 4) {
          break;
        }
      }
    //   std::cout<<"num now: "<<num_now<<" insert page id: "<<insert_page_id<<std::endl;
    //   fsm_page->ToString();
      auto page_next = fsm_page->GetChildId(num_now);
      pageid_t last_fsm_page = fsm_root_id_;
      insert_fsm_vec.push_back(fsm_page);
      bool is_fsm = true;
    //   std::cout<<"leaf"<<leaf<<std::endl;
      while(leaf != 1 || root == 1){
        // std::cout<<"page next: "<<page_next<<std::endl;
        if(page_next != NULL_PAGE_ID && page_next % 2 == 1){
          auto page_next_fsm = buffer_pool_.GetPage(db_oid_, oid_, page_next);
          auto fsm_page_next = std::make_shared<FSMPage>(page_next_fsm);
          insert_fsm_vec.push_back(fsm_page_next);
          int num_now = 0;
          for (int i = level * 4 + 3; i >= level * 4; i--) {
            num_now *= 2;
            num_now += tmp[i];
          }
          last_fsm_page = page_next;
          page_next = fsm_page_next->GetChildId(num_now);
          if (page_next % 2 == 0) {
            is_fsm = false;
            break;
          }
          leaf = fsm_page_next->GetLeaf();
          root = fsm_page_next->GetRoot();
          level++;
        }else{
            if(page_next != NULL_PAGE_ID) is_fsm = false;
            break;
        }
        
      }
      int level_now = level;
    //   std::cout<<"is fsm:"<<is_fsm<<"   level: "<<level<<std::endl;
    //   std::cout<<"fsm page: "<<page_next<<std::endl;
      if(is_fsm){
        if(page_next == NULL_PAGE_ID){
            // 可以直接插入
            // std::cout<<"is fsm page null: true "<<last_fsm_page<<std::endl;
            auto new_fsm_page = buffer_pool_.GetPage(db_oid_, oid_, last_fsm_page);
            // fsm_min_page_id_++;
            auto fsm_insert = std::make_shared<FSMPage>(new_fsm_page);
            fsm_insert->InsertPage(insert_page_id / 2, insert_page_free_size, 0);
            // insert_fsm_vec.push_back(fsm_insert);
        }
        level_now = 0;
        for(auto it = insert_fsm_vec.rbegin(); it != insert_fsm_vec.rend(); it++){
            (*it)->UpdatePage(insert_page_id / 2, insert_page_free_size, level_now);
            // (*it)->InsertPage(insert_page_id / 2, insert_page_free_size, level_now);
            level_now++;
        }
      }else{
        // std::cout<<"insert new fsm"<<std::endl;
        int tmp[33];
        auto cnt = fsm_page->ChangeToBinary(insert_page_id / 2, tmp);
        auto new_fsm_root = buffer_pool_.NewPage(db_oid_, oid_, fsm_min_page_id_ * 2 + 3);
        fsm_min_page_id_++;
        auto fsm_new_root = std::make_shared<FSMPage>(new_fsm_root);
        fsm_new_root->Init(1, 0);
        auto old_fsm_root = buffer_pool_.GetPage(db_oid_, oid_, fsm_root_id_);
        auto fsm_old_root = std::make_shared<FSMPage>(old_fsm_root);
        db_size_t old_root_max = fsm_old_root->GetMax();
        fsm_old_root->SetRoot(0);
        // std::cout<<"old_root_max : "<<old_root_max<<" fsm root :"<<fsm_root_id_ <<std::endl;
        fsm_new_root->InsertPage(0, old_root_max, 0);
        fsm_new_root->SetChildren(0, fsm_root_id_);
        // fsm_new_root->ToString();
        fsm_root_id_ = fsm_min_page_id_ * 2 + 1;
        int insert_level = 0;
        std::vector<std::shared_ptr<FSMPage>> new_fsm_vec;
        // std::cout<<"cnt: "<<cnt<<std::endl;
        // std::cout<<"insert new  begin"<<std::endl;
        for(int i = 0; i < cnt / 4; i++){
            // std::cout<<"insert new fsm: "<<fsm_min_page_id_*2+3<<std::endl;
            auto new_fsm = buffer_pool_.NewPage(db_oid_, oid_, fsm_min_page_id_* 2 + 3);
            fsm_min_page_id_++;
            auto fsm_new_page = std::make_shared<FSMPage>(new_fsm);
            fsm_new_page->Init(0, 0);
            if(i == 0){
                fsm_new_root->InsertPage(1, insert_page_free_size, insert_level);
                fsm_new_root->SetChildren(1, fsm_min_page_id_*2+1);
            }else{
                auto it = new_fsm_vec.rbegin();
                (*it)->InsertPage(0, insert_page_free_size, insert_level);
                (*it)->SetChildren(0, fsm_min_page_id_*2+1);
            }
            insert_level++;
            new_fsm_vec.push_back(fsm_new_page);
            // fsm_min_page_id_++;
        }
        auto it = new_fsm_vec.rbegin();
        // std::cout<<"new fsm is leaf: "<<(*it)->GetLeaf()<<std::endl;
        (*it)->SetLeaf(1);
        (*it)->InsertPage(insert_page_id / 2, insert_page_free_size, 0);
      }
    }
  }
  record->SetRid({insert_page_id,  insert_slot_id});
  return {insert_page_id,  insert_slot_id};
}

void Table::DeleteRecord(const Rid &rid, xid_t xid, bool write_log) {
  // 增加写 DeleteLog 过程
  // 设置页面的 page lsn
  // LAB 2 BEGIN

  // 使用 TablePage 操作页面
  // LAB 1 BEGIN
//   std::cout<<"delete table"<<std::endl;
  auto page_id = rid.page_id_;
  auto page = buffer_pool_.GetPage(db_oid_, oid_, page_id);
//   std::cout<<"delete table get page"<<std::endl;
  auto table_page = std::make_unique<TablePage>(page);
  table_page->DeleteRecord(rid.slot_id_, xid);
}

Rid Table::UpdateRecord(const Rid &rid, xid_t xid, cid_t cid, std::shared_ptr<Record> record, bool write_log) {
  DeleteRecord(rid, xid, write_log);
  return InsertRecord(record, xid, cid, write_log);
}

pageid_t Table::GetFirstPageId() const { return first_page_id_; }

oid_t Table::GetOid() const { return oid_; }

oid_t Table::GetDbOid() const { return db_oid_; }

const ColumnList &Table::GetColumnList() const { return column_list_; }

}  // namespace huadb
