#pragma once

#include "log/log_record.h"

namespace huadb {

class UndoLog : public LogRecord {
 public:
  UndoLog(lsn_t lsn, xid_t xid, lsn_t prev_lsn);

  size_t SerializeTo(char *data) const override;
  static std::shared_ptr<UndoLog> DeserializeFrom(lsn_t lsn, const char *data);

  std::string ToString() const override;
  int cnt = 0;
};

}  // namespace huadb
