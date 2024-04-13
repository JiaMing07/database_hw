#include "log/log_records/CLR_new_page_log.h"

namespace huadb {

CLRNewPageLog::CLRNewPageLog(lsn_t lsn, xid_t xid, lsn_t prev_lsn, oid_t oid, pageid_t prev_page_id, pageid_t page_id)
    : LogRecord(LogType::CLR_NEWPAGE, lsn, xid, prev_lsn), oid_(oid), prev_page_id_(prev_page_id), page_id_(page_id) {
  size_ += sizeof(oid_) + sizeof(page_id_) + sizeof(prev_page_id_);
}

size_t CLRNewPageLog::SerializeTo(char *data) const {
  size_t offset = LogRecord::SerializeTo(data);
  memcpy(data + offset, &oid_, sizeof(oid_));
  offset += sizeof(oid_);
  memcpy(data + offset, &prev_page_id_, sizeof(prev_page_id_));
  offset += sizeof(prev_page_id_);
  memcpy(data + offset, &page_id_, sizeof(page_id_));
  offset += sizeof(page_id_);
  assert(offset == size_);
  return offset;
}

std::shared_ptr<CLRNewPageLog> CLRNewPageLog::DeserializeFrom(lsn_t lsn, const char *data) {
  xid_t xid;
  lsn_t prev_lsn;
  oid_t oid;
  pageid_t page_id, prev_page_id;
  size_t offset = 0;
  memcpy(&xid, data + offset, sizeof(xid));
  offset += sizeof(xid);
  memcpy(&prev_lsn, data + offset, sizeof(prev_lsn));
  offset += sizeof(prev_lsn);
  memcpy(&oid, data + offset, sizeof(oid));
  offset += sizeof(oid);
  memcpy(&prev_page_id, data + offset, sizeof(prev_page_id));
  offset += sizeof(prev_page_id);
  memcpy(&page_id, data + offset, sizeof(page_id));
  offset += sizeof(page_id);
  return std::make_shared<CLRNewPageLog>(lsn, xid, prev_lsn, oid, prev_page_id, page_id);
}

// void CLRNewPageLog::Undo(BufferPool &buffer_pool, Catalog &catalog, LogManager &log_manager, lsn_t undo_next_lsn) {
  // LAB 2 
//   std::cout<<"undo new page"<<std::endl;
//   oid_t db_oid = catalog.GetDatabaseOid(oid_);
//   auto prev_page = buffer_pool.GetPage(db_oid, oid_, prev_page_id_);
//   auto prev_table_page =  std::make_unique<TablePage>(prev_page);
//   auto page = buffer_pool.GetPage(db_oid, oid_, page_id_);
//   auto table_page =  std::make_unique<TablePage>(page);
//   prev_table_page->SetNextPageId(table_page->GetNextPageId());
// }

void CLRNewPageLog::Redo(BufferPool &buffer_pool, Catalog &catalog, LogManager &log_manager) {
  // 如果 oid_ 不存在，表示该表已经被删除，无需 redo
  if (!catalog.TableExists(oid_)) {
    return;
  }
  // 根据日志信息进行重做
  // LAB 2 BEGIN
  std::cout<<"redo clr new page"<<std::endl;
    oid_t db_oid = catalog.GetDatabaseOid(oid_);
  auto prev_page = buffer_pool.GetPage(db_oid, oid_, prev_page_id_);
  auto prev_table_page =  std::make_unique<TablePage>(prev_page);
  auto page = buffer_pool.GetPage(db_oid, oid_, page_id_);
  auto table_page =  std::make_unique<TablePage>(page);
  prev_table_page->SetNextPageId(table_page->GetNextPageId());
}

oid_t CLRNewPageLog::GetOid() const { return oid_; }

pageid_t CLRNewPageLog::GetPageId() const { return page_id_; }

pageid_t CLRNewPageLog::GetPrevPageId() const { return prev_page_id_; }

std::string CLRNewPageLog::ToString() const {
  return fmt::format("CLRNewPageLog\t\t[{}\toid: {}\tprev_page_id: {}\tpage_id: {}]", LogRecord::ToString(), oid_,
                     prev_page_id_, page_id_);
}

}  // namespace huadb
