#include "table/table_scan.h"

#include "table/table_page.h"

namespace huadb {

TableScan::TableScan(BufferPool &buffer_pool, std::shared_ptr<Table> table, Rid rid)
    : buffer_pool_(buffer_pool), table_(std::move(table)), rid_(rid) {}

std::shared_ptr<Record> TableScan::GetNextRecord(xid_t xid, IsolationLevel isolation_level, cid_t cid,
                                                 const std::unordered_set<xid_t> &active_xids) {
  // 根据事务隔离级别及活跃事务集合，判断记录是否可见
  // LAB 3 BEGIN

  // 每次调用读取一条记录
  // 读取时更新 rid_ 变量，避免重复读取
  // 扫描结束时，返回空指针
  // 注意处理扫描空表的情况（rid_.page_id_ 为 NULL_PAGE_ID）
  // LAB 1 BEGIN
//   std::cout<<"Get Next record"<<std::endl;
  auto page_id = rid_.page_id_;
  if(page_id == NULL_PAGE_ID){
    return nullptr;
  }
//   std::cout<<"page_id： "<<page_id<<"  page_id  slot_id  "<<rid_.page_id_<<" "<<rid_.slot_id_<<std::endl;
  auto page = buffer_pool_.GetPage(table_->GetDbOid(), table_->GetOid(), page_id);
  auto table_page = std::make_unique<TablePage>(page);
  auto record = table_page->GetRecord(rid_, table_->GetColumnList());
  record->SetRid(rid_);
//   std::cout<<"record rid: "<<record->GetRid().page_id_<<" "<<record->GetRid().slot_id_<<std::endl;
  while(record->IsDeleted()){
    db_size_t record_count = table_page->GetRecordCount();
    // std::cout<<"record_count: "<<record_count<<std::endl;
    if(rid_.slot_id_ < record_count - 1){
        rid_.slot_id_ = rid_.slot_id_ + 1;
    }else{
        rid_.slot_id_ = 0;
        // std::cout<<"next page:"<<rid_.page_id_<<"     "<<table_page->GetNextPageId()<<std::endl;
        rid_.page_id_ = table_page->GetNextPageId();
        if(rid_.page_id_ == NULL_PAGE_ID){
            return nullptr;
        } else {
            page = buffer_pool_.GetPage(table_->GetDbOid(), table_->GetOid(), rid_.page_id_);
            table_page = std::make_unique<TablePage>(page);
        }
    }
    // std::cout<<"record rid: "<<record->GetRid().page_id_<<" "<<record->GetRid().slot_id_<<std::endl;
    record = table_page->GetRecord(rid_, table_->GetColumnList());
    // std::cout<<"rid: "<<rid_.page_id_<<" "<<rid_.slot_id_<<std::endl;
    record->SetRid(rid_);
  }
  db_size_t record_count = table_page->GetRecordCount();
  if(rid_.slot_id_ < record_count - 1){
    rid_.slot_id_ = rid_.slot_id_ + 1;
  }else{
    rid_.slot_id_ = 0;
    rid_.page_id_ = table_page->GetNextPageId();
  }
//   std::cout<<"record： "<<record->ToString()<<" "<<record->IsDeleted()<<std::endl;
  return record;
}

}  // namespace huadb
