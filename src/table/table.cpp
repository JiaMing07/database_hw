#include "table/table.h"

#include "table/table_page.h"

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
  } else {
    first_page_id_ = 0;
  }
}

Rid Table::InsertRecord(std::shared_ptr<Record> record, xid_t xid, cid_t cid, bool write_log) {
  if (record->GetSize() > MAX_RECORD_SIZE) {
    throw DbException("Record size too large: " + std::to_string(record->GetSize()));
  }
//   std::cout<<"insert record"<<std::endl;
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
  //    std::cout<<first_page_id_<<std::endl;
  if(first_page_id_ == NULL_PAGE_ID){
    auto page = buffer_pool_.NewPage(db_oid_, oid_, 0);
    first_page_id_ = 0;
    auto table_page = std::make_unique<TablePage>(page);
    table_page->Init();
    insert_slot_id = table_page->InsertRecord(record, xid, cid);
    insert_page_id = 0;
  } else {
    auto page = buffer_pool_.GetPage(db_oid_, oid_, first_page_id_);
    bool flag = false;
    auto table_page = std::make_unique<TablePage>(page);
    if(table_page->GetFreeSpaceSize() >= record->GetSize()){
        table_page->InsertRecord(record, xid, cid);
        flag = true;
    }else{
        auto next_page_id = table_page->GetNextPageId();
        auto last_page_id = first_page_id_;
        // std::cout<<"last next: "<<last_page_id<<" "<<next_page_id<<std::endl;
        while(next_page_id != NULL_PAGE_ID){
            auto next_page = buffer_pool_.GetPage(db_oid_, oid_,next_page_id);
            auto next_table_page = std::make_unique<TablePage>(next_page);
            if(next_table_page->GetFreeSpaceSize() >= record->GetSize()){
                insert_page_id = next_page_id;
                insert_slot_id = next_table_page->InsertRecord(record, xid, cid);
                flag = true;
                break;
            }
            last_page_id = next_page_id;
            next_page_id = next_table_page->GetNextPageId();
            // std::cout<<"last next: "<<last_page_id<<" "<<next_page_id<<std::endl;
            if(next_page_id == NULL_PAGE_ID){
                next_table_page->SetNextPageId(last_page_id + 1);
            }
        }
        if(flag == false){
            if(last_page_id == first_page_id_){
                // std::cout<<"set first next page id"<<std::endl;
                table_page->SetNextPageId(first_page_id_ + 1);
            }
            // std::cout<<"last page id: "<<last_page_id<<" make new page"<<std::endl;
            auto new_page = buffer_pool_.NewPage(db_oid_, oid_, last_page_id + 1);
            auto new_table_page = std::make_unique<TablePage>(new_page);
            new_table_page->Init();
            insert_page_id = last_page_id + 1;
            insert_slot_id = new_table_page->InsertRecord(record, xid, cid);
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
//   std::cout<<"first page id: "<<first_page_id_<<std::endl;
//   std::cout<<page_id<<" "<<rid.slot_id_<<std::endl;
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
