#include "log/log_records/undo_log.h"

namespace huadb {

UndoLog::UndoLog(lsn_t lsn, xid_t xid, lsn_t prev_lsn) : LogRecord(LogType::UNDOCRASH, lsn, xid, prev_lsn) {
    std::cout<<"make undo log"<<std::endl;
}

size_t UndoLog::SerializeTo(char *data) const {
  size_t offset = LogRecord::SerializeTo(data);
  assert(offset == size_);
  return offset;
}

std::shared_ptr<UndoLog> UndoLog::DeserializeFrom(lsn_t lsn, const char *data) {
  xid_t xid;
  lsn_t prev_lsn;
  size_t offset = 0;
  memcpy(&xid, data + offset, sizeof(xid));
  offset += sizeof(xid);
  memcpy(&prev_lsn, data + offset, sizeof(prev_lsn));
  offset += sizeof(prev_lsn);
  return std::make_shared<UndoLog>(lsn, xid, prev_lsn);
}

std::string UndoLog::ToString() const { return fmt::format("UndoLog\t\t[{}]", LogRecord::ToString()); }

}  // namespace huadb
