#include "log/log_records/CLR_delete_log.h"

namespace huadb {

CLRDeleteLog::CLRDeleteLog(lsn_t lsn, xid_t xid, lsn_t prev_lsn, oid_t oid, pageid_t page_id, slotid_t slot_id)
    : LogRecord(LogType::CLR_DELETE, lsn, xid, prev_lsn), oid_(oid), page_id_(page_id), slot_id_(slot_id) {
  size_ += sizeof(oid_) + sizeof(page_id_) + sizeof(slot_id_);
}

size_t CLRDeleteLog::SerializeTo(char *data) const {
  size_t offset = LogRecord::SerializeTo(data);
  memcpy(data + offset, &oid_, sizeof(oid_));
  offset += sizeof(oid_);
  memcpy(data + offset, &page_id_, sizeof(page_id_));
  offset += sizeof(page_id_);
  memcpy(data + offset, &slot_id_, sizeof(slot_id_));
  offset += sizeof(slot_id_);
  assert(offset == size_);
  return offset;
}

std::shared_ptr<CLRDeleteLog> CLRDeleteLog::DeserializeFrom(lsn_t lsn, const char *data) {
  xid_t xid;
  lsn_t prev_lsn;
  oid_t oid;
  pageid_t page_id;
  slotid_t slot_id;
  size_t offset = 0;
  memcpy(&xid, data + offset, sizeof(xid));
  offset += sizeof(xid);
  memcpy(&prev_lsn, data + offset, sizeof(prev_lsn));
  offset += sizeof(prev_lsn);
  memcpy(&oid, data + offset, sizeof(oid));
  offset += sizeof(oid);
  memcpy(&page_id, data + offset, sizeof(page_id));
  offset += sizeof(page_id);
  memcpy(&slot_id, data + offset, sizeof(slot_id));
  offset += sizeof(slot_id);
  return std::make_shared<CLRDeleteLog>(lsn, xid, prev_lsn, oid, page_id, slot_id);
}

// void CLRDeleteLog::Undo(BufferPool &buffer_pool, Catalog &catalog, LogManager &log_manager, lsn_t undo_next_lsn) {
//   // 恢复删除的记录
//   // 通过 catalog_ 获取 db_oid
//   // LAB 2 BEGIN
//   oid_t db_oid = catalog.GetDatabaseOid(oid_);
//   auto page = buffer_pool.GetPage(db_oid, oid_, page_id_);
//   auto table_page =  std::make_unique<TablePage>(page);
//   table_page->UndoDeleteRecord(slot_id_);
// //   log_manager.AppendRollbackLog(xid_);
// }

void CLRDeleteLog::Redo(BufferPool &buffer_pool, Catalog &catalog, LogManager &log_manager) {
  // 如果 oid_ 不存在，表示该表已经被删除，无需 redo
  if (!catalog.TableExists(oid_)) {
    return;
  }
  // 根据日志信息进行重做
  // LAB 2 BEGIN
  std::cout<<"redo clr delete log"<<std::endl;
  oid_t db_oid = catalog.GetDatabaseOid(oid_);
  auto page = buffer_pool.GetPage(db_oid, oid_, page_id_);
  auto table_page =  std::make_unique<TablePage>(page);
  table_page->UndoDeleteRecord(slot_id_);
}

oid_t CLRDeleteLog::GetOid() const { return oid_; }

pageid_t CLRDeleteLog::GetPageId() const { return page_id_; }

std::string CLRDeleteLog::ToString() const {
  return fmt::format("CLRDeleteLog\t\t[{} oid: {}\tpage_id: {}\tslot_id: {}]", LogRecord::ToString(), oid_, page_id_,
                     slot_id_);
}

}  // namespace huadb
