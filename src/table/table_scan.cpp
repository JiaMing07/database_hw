#include "table/table_scan.h"

#include "table/table_page.h"

namespace huadb {

TableScan::TableScan(BufferPool &buffer_pool, std::shared_ptr<Table> table, Rid rid)
    : buffer_pool_(buffer_pool), table_(std::move(table)), rid_(rid) {}

bool TableScan::CanGet(std::shared_ptr<Record> record, xid_t xid, cid_t cid, IsolationLevel isolation_level, const std::unordered_set<xid_t> &active_xids){
    // std::cout<<"record rid: "<<record->GetRid().page_id_<<" "<<record->GetRid().slot_id_<<std::endl;
    // std::cout<<"record cid: "<<record->GetCid()<<"    cid:"<<cid<<"   xid: "<<xid<<std::endl;
    // std::cout<<"record xid: "<<record->GetXmin()<<"   "<<record->GetXmax()<<std::endl;
    // std::cout<<"active xids: ";
    // for(auto it:active_xids){
    //     std::cout<<it<<" ";
    // }
    // std::cout<<std::endl;
    if(isolation_level == IsolationLevel::READ_COMMITTED){
        // std::cout<<"read commit"<<std::endl;
        if((record->GetCid() == cid) && (record->GetXmin() == xid)){
            return true;
        }else{
            // std::cout<<record->GetXmin()<<" "<<xid<<std::endl;
            auto find_in_active = active_xids.find(record->GetXmin());
            // std::cout<<(find_in_active != active_xids.end())<<std::endl;
            if(find_in_active != active_xids.end() && record->GetXmin() != xid){
                // 插入不可见
                // std::cout<<"in"<<std::endl;
                return true;
            }else if(find_in_active != active_xids.end() && record->GetXmin() == xid){
                // 当前 xid 插入，插入可见
                if(xid == record->GetXmax() && record->GetXmax() != NULL_XID){
                    return true;
                }
                auto find_xmax_in_active = active_xids.find(record->GetXmax());
                // std::cout<<"find xmax: "<<(find_xmax_in_active == active_xids.end())<<std::endl;
                if(record->GetXmax() != NULL_XID && find_xmax_in_active == active_xids.end()){
                    // 删除可见（已提交）
                    return true;
                }else{
                    return false;
                }
            }else if(find_in_active == active_xids.end()){
                // 插入可见，检查删除
                if(record->GetXmax() == xid && record->GetXmax() != NULL_XID){
                    return true;
                }
                auto find_xmax_in_active = active_xids.find(record->GetXmax());
                // std::cout<<"find xmax: "<<(find_xmax_in_active == active_xids.end())<<std::endl;
                if(record->GetXmax() != NULL_XID && find_xmax_in_active == active_xids.end()){
                    // 删除可见（已提交）
                    return true;
                }else{
                    // std::cout<<"false"<<std::endl;
                    return false;
                }
            }
        }
        return false;
    }else if(isolation_level == IsolationLevel::REPEATABLE_READ){
        if((record->GetCid() == cid) && (record->GetXmin() == xid)){
            return true;
        }else{
            // std::cout<<record->GetXmin()<<" "<<xid<<std::endl;
            auto find_in_active = active_xids.find(record->GetXmin());
            std::cout<<(find_in_active != active_xids.end())<<std::endl;
            if(find_in_active != active_xids.end() && record->GetXmin() != xid){
                // std::cout<<"in"<<std::endl;
                return true;
            }else if(find_in_active != active_xids.end() && record->GetXmin() == xid){
                if(record->GetXmax() == xid && record->GetXmax() != NULL_XID){
                    return true;
                }else{
                    return false;
                }
            }else if(find_in_active == active_xids.end() && record->GetXmin() <= xid){
                // 插入是可见的，判断删除是否可见
                if(record->GetXmax() == xid && record->GetXmax() != NULL_XID){
                    return true;
                }
                auto find_xmax_in_active = active_xids.find(record->GetXmax());
                if(find_xmax_in_active != active_xids.end() || record->GetXmax() > xid){
                    // 删除不可见，在当前事务下仍然是可见的
                    return false;
                }else{
                    return true;
                }
            }else if(find_in_active == active_xids.end() && record->GetXmin() > xid){
                // 之后产生的事务插入，当前事务不可见
                return true;
            }
        }
        return false;
    }else if(isolation_level == IsolationLevel::SERIALIZABLE){
        // std::cout<<(record->GetXmax() != xid || (record->GetCid() == cid && record->GetXmin() == xid))<<std::endl;
        auto find_xmax_in_active = active_xids.find(record->GetXmax());
        auto find_xmin_in_active = active_xids.find(record->GetXmin());
        return (record->GetXmin() != xid && find_xmin_in_active != active_xids.end()) || ((record->GetXmax() == xid) || (record->GetCid() == cid && record->GetXmin() == xid) || (record->GetXmax() != NULL_XID && find_xmax_in_active == active_xids.end()));
    }
}

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
//   std::cout<<"record cid: "<<record->GetCid()<<"    cid:"<<cid<<"   xid: "<<xid<<std::endl;
//   std::cout<<"record xid: "<<record->GetXmin()<<"   "<<record->GetXmax()<<std::endl;
//   std::cout<<"active xids: ";
//   for(auto it:active_xids){
//     std::cout<<it<<" ";
//   }
//   std::cout<<std::endl;
  while(CanGet(record, xid, cid, isolation_level, active_xids)){
    db_size_t record_count = table_page->GetRecordCount();
    // std::cout<<"record_count: "<<record_count<<std::endl;
    if(rid_.slot_id_ < record_count - 1){
        rid_.slot_id_ = rid_.slot_id_ + 1;
    }else{
        rid_.slot_id_ = 0;
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
