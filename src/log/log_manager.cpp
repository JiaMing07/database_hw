#include "log/log_manager.h"

#include "common/exceptions.h"
#include "log/log_records/log_records.h"

namespace huadb {

LogManager::LogManager(Disk &disk, TransactionManager &transaction_manager, lsn_t next_lsn)
    : disk_(disk), transaction_manager_(transaction_manager), next_lsn_(next_lsn), flushed_lsn_(next_lsn - 1) {}

void LogManager::SetBufferPool(std::shared_ptr<BufferPool> buffer_pool) { buffer_pool_ = std::move(buffer_pool); }

void LogManager::SetCatalog(std::shared_ptr<Catalog> catalog) { catalog_ = std::move(catalog); }

lsn_t LogManager::GetNextLSN() const { return next_lsn_; }

void LogManager::Clear() {
  std::unique_lock lock(log_buffer_mutex_);
  log_buffer_.clear();
}

void LogManager::Flush() { Flush(NULL_LSN); }

void LogManager::SetDirty(oid_t oid, pageid_t page_id, lsn_t lsn) {
  if (dpt_.find({oid, page_id}) == dpt_.end()) {
    dpt_[{oid, page_id}] = lsn;
  }
}

lsn_t LogManager::AppendInsertLog(xid_t xid, oid_t oid, pageid_t page_id, slotid_t slot_id, db_size_t offset,
                                  db_size_t size, char *new_record) {
  std::cout<<"inset log append:"<<next_lsn_<<std::endl;
  if (att_.find(xid) == att_.end()) {
    throw DbException(std::to_string(xid) + " does not exist in att (in AppendInsertLog)");
  }
  auto log = std::make_shared<InsertLog>(NULL_LSN, xid, att_.at(xid), oid, page_id, slot_id, offset, size, new_record);
  lsn_t lsn = next_lsn_.fetch_add(log->GetSize(), std::memory_order_relaxed);
  log->SetLSN(lsn);
  att_[xid] = lsn;
  {
    std::unique_lock lock(log_buffer_mutex_);
    log_buffer_.push_back(std::move(log));
  }
  if (dpt_.find({oid, page_id}) == dpt_.end()) {
    dpt_[{oid, page_id}] = lsn;
  }
  return lsn;
}

lsn_t LogManager::AppendDeleteLog(xid_t xid, oid_t oid, pageid_t page_id, slotid_t slot_id) {
  std::cout<<"append delete log: "<<next_lsn_<<std::endl;
  if (att_.find(xid) == att_.end()) {
    // std::cerr<<"append delete log: "<<xid<<std::endl;
    throw DbException(std::to_string(xid) + " does not exist in att (in AppendDeleteLog)");
  }
  auto log = std::make_shared<DeleteLog>(NULL_LSN, xid, att_.at(xid), oid, page_id, slot_id);
  lsn_t lsn = next_lsn_.fetch_add(log->GetSize(), std::memory_order_relaxed);
  log->SetLSN(lsn);
  att_[xid] = lsn;
  {
    std::unique_lock lock(log_buffer_mutex_);
    log_buffer_.push_back(std::move(log));
  }
  if (dpt_.find({oid, page_id}) == dpt_.end()) {
    dpt_[{oid, page_id}] = lsn;
  }
  return lsn;
}

lsn_t LogManager::AppendNewPageLog(xid_t xid, oid_t oid, pageid_t prev_page_id, pageid_t page_id) {
  std::cout<<"new page log append: "<<next_lsn_<<std::endl;
  if (xid != DDL_XID && att_.find(xid) == att_.end()) {
    throw DbException(std::to_string(xid) + " does not exist in att (in AppendNewPageLog)");
  }
  xid_t log_xid;
  if (xid == DDL_XID) {
    log_xid = NULL_XID;
  } else {
    log_xid = att_.at(xid);
  }
  auto log = std::make_shared<NewPageLog>(NULL_LSN, xid, log_xid, oid, prev_page_id, page_id);
  lsn_t lsn = next_lsn_.fetch_add(log->GetSize(), std::memory_order_relaxed);
  log->SetLSN(lsn);

  if (xid != DDL_XID) {
    att_[xid] = lsn;
  }
  {
    std::unique_lock lock(log_buffer_mutex_);
    log_buffer_.push_back(std::move(log));
  }
  if (dpt_.find({oid, page_id}) == dpt_.end()) {
    dpt_[{oid, page_id}] = lsn;
  }
  if (prev_page_id != NULL_PAGE_ID && dpt_.find({oid, prev_page_id}) == dpt_.end()) {
    dpt_[{oid, prev_page_id}] = lsn;
  }
  return lsn;
}

lsn_t LogManager::AppendBeginLog(xid_t xid) {
    std::cout<<"begin log append: "<<next_lsn_<<std::endl;
  if (att_.find(xid) != att_.end()) {
    throw DbException(std::to_string(xid) + " already exists in att");
  }
  auto log = std::make_shared<BeginLog>(NULL_LSN, xid, NULL_LSN);
  lsn_t lsn = next_lsn_.fetch_add(log->GetSize(), std::memory_order_relaxed);
  log->SetLSN(lsn);
  att_[xid] = lsn;
  {
    std::unique_lock lock(log_buffer_mutex_);
    log_buffer_.push_back(std::move(log));
  }
  std::cout<<"begin finish"<<std::endl;
  return lsn;
}

lsn_t LogManager::AppendCommitLog(xid_t xid) {
    std::cout<<"commit log append: "<<next_lsn_<<std::endl;
  if (att_.find(xid) == att_.end()) {
    throw DbException(std::to_string(xid) + " does not exist in att (in AppendCommitLog)");
  }
  auto log = std::make_shared<CommitLog>(NULL_LSN, xid, att_.at(xid));
  lsn_t lsn = next_lsn_.fetch_add(log->GetSize(), std::memory_order_relaxed);
  log->SetLSN(lsn);
  {
    std::unique_lock lock(log_buffer_mutex_);
    log_buffer_.push_back(std::move(log));
  }
  Flush(lsn);
  att_.erase(xid);
  std::cout<<"commit finish"<<std::endl;
  return lsn;
}

lsn_t LogManager::AppendRollbackLog(xid_t xid) {
    std::cout<<"rollback log append: "<<next_lsn_<<std::endl;
  if (att_.find(xid) == att_.end()) {
    throw DbException(std::to_string(xid) + " does not exist in att (in AppendRollbackLog)");
  }
  auto log = std::make_shared<RollbackLog>(NULL_LSN, xid, att_.at(xid));
  lsn_t lsn = next_lsn_.fetch_add(log->GetSize(), std::memory_order_relaxed);
  log->SetLSN(lsn);
  {
    std::unique_lock lock(log_buffer_mutex_);
    log_buffer_.push_back(std::move(log));
  }
  Flush(lsn);
  att_.erase(xid);
  return lsn;
}

void WriteLog2(lsn_t lsn, size_t log_size, char* log_ptr, Disk &disk_, lsn_t begin_lsn){
    std::cout<<"begin write log2"<<std::endl;
    std::this_thread::sleep_for(std::chrono::microseconds(2000));
    disk_.WriteLog(lsn, log_size, log_ptr);
    delete[] log_ptr;
    std::ofstream out(MASTER_RECORD_NAME);
    out << begin_lsn;
    std::cout<<"finish write log2"<<std::endl;
}

void WriteLog1(lsn_t lsn_begin, size_t log_size_begin, char* log_ptr_begin, Disk &disk_){
    disk_.WriteLog(lsn_begin, log_size_begin, log_ptr_begin);
    delete[] log_ptr_begin;
    std::cout<<"finish write log1"<<std::endl;
}

lsn_t LogManager::Checkpoint(bool async) {
//   Flush(next_lsn_);
std::cout<<"checkpoint begin"<<std::endl;
    if(async == false){
        auto begin_checkpoint_log = std::make_shared<BeginCheckpointLog>(NULL_LSN, NULL_XID, NULL_LSN);
        lsn_t begin_lsn = next_lsn_.fetch_add(begin_checkpoint_log->GetSize(), std::memory_order_relaxed);
        begin_checkpoint_log->SetLSN(begin_lsn);
        {
            std::unique_lock lock(log_buffer_mutex_);
            log_buffer_.push_back(std::move(begin_checkpoint_log));
        }
        auto end_checkpoint_log = std::make_shared<EndCheckpointLog>(NULL_LSN, NULL_XID, NULL_LSN, att_, dpt_);
        lsn_t end_lsn = next_lsn_.fetch_add(end_checkpoint_log->GetSize(), std::memory_order_relaxed);
        end_checkpoint_log->SetLSN(end_lsn);
        {
            std::unique_lock lock(log_buffer_mutex_);
            log_buffer_.push_back(std::move(end_checkpoint_log));
        }
        std::cout<<"flush end_lsn: "<<end_lsn<<std::endl;
        Flush(end_lsn);
        std::ofstream out(MASTER_RECORD_NAME);
        out << begin_lsn;
        return end_lsn;
    }else{
        std::cout<<"async"<<std::endl;
        auto begin_checkpoint_log = std::make_shared<BeginCheckpointLog>(NULL_LSN, NULL_XID, NULL_LSN);
        lsn_t begin_lsn = next_lsn_.fetch_add(begin_checkpoint_log->GetSize(), std::memory_order_relaxed);
        begin_checkpoint_log->SetLSN(begin_lsn);
        auto log_size = begin_checkpoint_log->GetSize();
        auto log = new char[log_size];
        begin_checkpoint_log->SerializeTo(log);
        std::cout<<"thread t1:"<<begin_checkpoint_log->GetLSN()<<std::endl;
        std::thread t1(WriteLog1, begin_checkpoint_log->GetLSN(), log_size, log, std::ref(disk_));
        // disk_.WriteLog(begin_checkpoint_log->GetLSN(), log_size, log);
        // delete[] log;
        t1.detach();
        std::cout<<"thread t2: "<<std::endl;
        auto end_checkpoint_log = std::make_shared<EndCheckpointLog>(NULL_LSN, NULL_XID, NULL_LSN, att_, dpt_);
        lsn_t end_lsn = next_lsn_.fetch_add(end_checkpoint_log->GetSize(), std::memory_order_relaxed);
        end_checkpoint_log->SetLSN(end_lsn);
        log_size = end_checkpoint_log->GetSize();
        auto log_end = new char[log_size];
        end_checkpoint_log->SerializeTo(log_end);
        std::cout<<"thread t2: "<<end_checkpoint_log->GetLSN()<<std::endl;
        std::thread t2(WriteLog2, end_checkpoint_log->GetLSN(), log_size, log_end, std::ref(disk_), begin_lsn);
        t2.detach();
        std::cout<<"flush end_lsn: "<<end_lsn<<std::endl;
        // Flush(end_lsn);
        // std::ofstream out(MASTER_RECORD_NAME);
        // out << begin_lsn;
        return 0;
    }

}

void LogManager::FlushPage(oid_t table_oid, pageid_t page_id, lsn_t page_lsn) {
  Flush(page_lsn);
  dpt_.erase({table_oid, page_id});
}

void LogManager::Rollback(xid_t xid) {
  // 在 att_ 中查找事务 xid 的最后一条日志的 lsn
  // 依次获取 lsn 的 prev_lsn_，直到 NULL_LSN
  // 根据 lsn 和 flushed_lsn_ 的大小关系，判断日志在 buffer 中还是在磁盘中
  // 若日志在 buffer 中，通过 log_buffer_ 获取日志
  // 若日志在磁盘中，通过 disk_ 读取日志，count 参数可设置为 MAX_LOG_SIZE
  // 通过 LogRecord::DeserializeFrom 函数解析日志
  // 调用日志的 Undo 函数
  // LAB 2 BEGIN

  lsn_t last_lsn = att_[xid];
  lsn_t lsn_now = last_lsn;
//   std::cout<<"last lsn: "<<last_lsn<<"  flushed_lsn_: "<<flushed_lsn_<<std::endl;
  while(lsn_now != NULL_LSN){
    bool in_buffer = false;
    if(lsn_now > flushed_lsn_){
        in_buffer = true;
    }
    if(in_buffer){
        for(auto it : log_buffer_){
            if(it->GetLSN() == lsn_now){
                it->Undo(*buffer_pool_, *catalog_, *this, it->GetPrevLSN());
                lsn_now = it->GetPrevLSN();
                break;
            }
        }
    }else{
        // std::cout<<"lsn now: "<<lsn_now<<"  flushed_lsn_: "<<flushed_lsn_<<"    count: "<<MAX_LOG_SIZE<<std::endl;
        // std::unique_ptr<char> log(new char[MAX_LOG_SIZE]);
        char* get_log = new char[MAX_LOG_SIZE];
        disk_.ReadLog(lsn_now, MAX_LOG_SIZE, get_log);
        std::shared_ptr<LogRecord> log_record = LogRecord::DeserializeFrom(lsn_now, get_log);
        lsn_now = log_record->GetPrevLSN();
        log_record->Undo(*buffer_pool_, *catalog_, *this, log_record->GetPrevLSN());
        delete[] get_log;
    }
  }
}

void LogManager::Recover() {
  Analyze();
  Redo();
  Undo();
}

void LogManager::IncrementRedoCount() { redo_count_++; }

uint32_t LogManager::GetRedoCount() const { return redo_count_; }

void LogManager::Flush(lsn_t lsn) {
    std::cout<<"log manager flush: "<<lsn<<std::endl;
  size_t max_log_size = 0;
  lsn_t max_lsn = NULL_LSN;
  {
    std::unique_lock lock(log_buffer_mutex_);
    for (auto iterator = log_buffer_.cbegin(); iterator != log_buffer_.cend();) {
      const auto &log_record = *iterator;
      // 如果 lsn 为 NULL_LSN，表示 log_buffer_ 中所有日志都需要刷盘
      if (lsn != NULL_LSN && log_record->GetLSN() > lsn) {
        iterator++;
        continue;
      }
      auto log_size = log_record->GetSize();
      auto log = std::make_unique<char[]>(log_size);
      log_record->SerializeTo(log.get());
      disk_.WriteLog(log_record->GetLSN(), log_size, log.get());
      if (max_lsn == NULL_LSN || log_record->GetLSN() > max_lsn) {
        max_lsn = log_record->GetLSN();
        max_log_size = log_size;
      }
      iterator = log_buffer_.erase(iterator);
    }
  }
  // 如果 max_lsn 为 NULL_LSN，表示没有日志刷盘
  // 如果 flushed_lsn_ 为 NULL_LSN，表示还没有日志刷过盘
  if (max_lsn != NULL_LSN && (flushed_lsn_ == NULL_LSN || max_lsn > flushed_lsn_)) {
    flushed_lsn_ = max_lsn;
    lsn_t next_lsn = FIRST_LSN;
    if (disk_.FileExists(NEXT_LSN_NAME)) {
      std::ifstream in(NEXT_LSN_NAME);
      in >> next_lsn;
    }
    if (flushed_lsn_ + max_log_size > next_lsn) {
      std::ofstream out(NEXT_LSN_NAME);
      out << (flushed_lsn_ + max_log_size);
    }
  }
  std::cout<<"log manager flush finish: "<<lsn<<std::endl;
}

void LogManager::Analyze() {
  // 恢复 Master Record 等元信息
  // 恢复故障时正在使用的数据库
  std::cout<<"analyze"<<std::endl;
  if (disk_.FileExists(NEXT_LSN_NAME)) {
    std::ifstream in(NEXT_LSN_NAME);
    lsn_t next_lsn;
    in >> next_lsn;
    next_lsn_ = next_lsn;
  } else {
    next_lsn_ = FIRST_LSN;
  }
  flushed_lsn_ = next_lsn_ - 1;
  lsn_t checkpoint_lsn = 0;

  if (disk_.FileExists(MASTER_RECORD_NAME)) {
    std::ifstream in(MASTER_RECORD_NAME);
    in >> checkpoint_lsn;
  }
  // 根据 Checkpoint 日志恢复脏页表、活跃事务表等元信息
  // 必要时调用 transaction_manager_.SetNextXid 来恢复事务 id
  // LAB 2 BEGIN
  xid_t xid_max = 0;
  char *begin_log = new char[next_lsn_ - checkpoint_lsn];disk_.ReadLog(checkpoint_lsn, next_lsn_ - checkpoint_lsn, begin_log);
  std::shared_ptr<BeginLog> begin_log_record = BeginLog::DeserializeFrom(checkpoint_lsn, begin_log);
  delete[] begin_log;
  checkpoint_lsn += begin_log_record->GetSize();char *get_log = new char[next_lsn_ - checkpoint_lsn];
  disk_.ReadLog(checkpoint_lsn, next_lsn_ - checkpoint_lsn, get_log);
  std::shared_ptr<EndCheckpointLog> log_record = EndCheckpointLog::DeserializeFrom(checkpoint_lsn, get_log + sizeof(LogType));
  auto checkpoint_att = log_record->GetATT();
  auto checkpoint_dpt = log_record->GetDPT();
  lsn_t lsn_now = checkpoint_lsn + log_record->GetSize();
  delete[] get_log;
  while(lsn_now < next_lsn_){
    char *log_now = new char[MAX_LOG_SIZE];
    disk_.ReadLog(lsn_now, MAX_LOG_SIZE, log_now);
    std::shared_ptr<LogRecord> log_record_now = LogRecord::DeserializeFrom(lsn_now, log_now);
    xid_t xid_now = log_record_now->GetXid();
    if(log_record_now->GetType() == LogType::COMMIT || log_record_now->GetType() == LogType::ROLLBACK){
        if(checkpoint_att.find(xid_now) != checkpoint_att.end()){
            checkpoint_att.erase(xid_now);
        }
    }else{
        checkpoint_att[xid_now] = log_record_now->GetLSN();
    }
    LogType log_record_type = log_record_now->GetType();
    if(log_record_type == LogType::INSERT || log_record_type == LogType::DELETE || log_record_type == LogType::NEW_PAGE){
        oid_t table_oid;
        pageid_t page_id;
        // std::cout<<"lsn now: "<<lsn_now<<"  log now size: "<<log_record_now->GetSize()<<std::endl;
        if(log_record_type == LogType::INSERT){
            char *insert_log_now = new char[MAX_LOG_SIZE];
            disk_.ReadLog(lsn_now, MAX_LOG_SIZE, insert_log_now);
            std::shared_ptr<InsertLog> insert_log = InsertLog::DeserializeFrom(lsn_now, insert_log_now + sizeof(LogType));
            table_oid = insert_log->GetOid();
            page_id = insert_log->GetPageId();
            delete[] insert_log_now;
        }else if (log_record_type == LogType::DELETE){
            char *delete_log_now = new char[MAX_LOG_SIZE];
            disk_.ReadLog(lsn_now, MAX_LOG_SIZE, delete_log_now);
            std::shared_ptr<DeleteLog> delete_log = DeleteLog::DeserializeFrom(lsn_now, delete_log_now + sizeof(LogType));
            table_oid = delete_log->GetOid();
            page_id = delete_log ->GetPageId();
            delete[] delete_log_now;
        }else if(log_record_type == LogType::NEW_PAGE){
            char *newpage_log_now = new char[MAX_LOG_SIZE];
            disk_.ReadLog(lsn_now, MAX_LOG_SIZE, newpage_log_now);
            std::shared_ptr<NewPageLog> newpage_log = NewPageLog::DeserializeFrom(lsn_now, newpage_log_now + sizeof(LogType));
            table_oid = newpage_log->GetOid();
            page_id = newpage_log->GetPageId();
            delete[] newpage_log_now;
        }
        
        // std::shared_ptr<InsertLog> insert_log = InsertLog::DeserializeFrom(lsn_now, log_now);
        if(checkpoint_dpt.find({table_oid, page_id}) == checkpoint_dpt.end()){
            checkpoint_dpt[{table_oid, page_id}] = lsn_now;
        }
    }
    if(log_record_now->GetXid() > xid_max){
        xid_max = log_record_now->GetXid();
    }
    lsn_now = lsn_now + log_record_now->GetSize();
    
    delete[] log_now;
  }
  transaction_manager_.SetNextXid(xid_max + 1);
  att_ = checkpoint_att;
  dpt_ = checkpoint_dpt;
}

void LogManager::Redo() {
  // 正序读取日志，调用日志记录的 Redo 函数
  // LAB 2 BEGIN
  std::cout<<"log manager redo"<<std::endl;
  lsn_t lsn_now = 0;
  lsn_t checkpoint_lsn = 0;

  if (disk_.FileExists(MASTER_RECORD_NAME)) {
    std::ifstream in(MASTER_RECORD_NAME);
    in >> checkpoint_lsn;
  }
  lsn_t min_lsn = next_lsn_;
  for(auto it:dpt_){
    if(it.second < min_lsn){
        min_lsn = it.second;
    }
  }
  if(min_lsn == next_lsn_){
    min_lsn = checkpoint_lsn;
  }
  lsn_now = min_lsn;
  while (lsn_now < next_lsn_) {
    bool in_buffer = false;
    if (lsn_now > flushed_lsn_) {
      in_buffer = true;
    }
    if (in_buffer) {
      for (auto it : log_buffer_) {
        if (it->GetLSN() == lsn_now) {
            oid_t table_oid;
            pageid_t page_id;
            if(it->GetType() == LogType::INSERT){
                auto insert_ptr = std::dynamic_pointer_cast<InsertLog>(it);
                table_oid = insert_ptr->GetOid();
                page_id = insert_ptr->GetPageId();
            }else if(it->GetType() == LogType::DELETE){
                auto delete_ptr = std::dynamic_pointer_cast<DeleteLog>(it);
                table_oid = delete_ptr->GetOid();
                page_id = delete_ptr->GetPageId();
            }else if(it->GetType() == LogType::NEW_PAGE){
                auto newpage_ptr = std::dynamic_pointer_cast<DeleteLog>(it);
                table_oid = newpage_ptr->GetOid();
                page_id = newpage_ptr->GetPageId();
                
            }else{
                lsn_now = lsn_now + it->GetSize();
                break;
            }
            if(dpt_.find({table_oid, page_id}) == dpt_.end()){
                lsn_now = lsn_now + it->GetSize();
                break;
            }else{
                lsn_t rec_lsn = dpt_[{table_oid, page_id}];
                if(it->GetLSN() <= rec_lsn){
                    lsn_now = lsn_now + it->GetSize();
                    break;
                }else{
                    if(it->GetType() == LogType::NEW_PAGE){
                        it->Redo(*buffer_pool_, *catalog_, *this);
                        lsn_now = lsn_now + it->GetSize();
                    }else{
                        auto page = buffer_pool_->GetPage(catalog_->GetDatabaseOid(table_oid), table_oid, page_id);
                        auto table_page = std::make_shared<TablePage>(page);
                        auto page_lsn = table_page->GetPageLSN();
                        if(lsn_now < page_lsn){
                            lsn_now = lsn_now + it->GetSize();
                            break;
                        }else{
                            it->Redo(*buffer_pool_, *catalog_, *this);
                            lsn_now = lsn_now + it->GetSize();
                        }
                    }
                }
            }
            // it->Redo(*buffer_pool_, * catalog_, *this);
            break;
        }
      }
    } else {
    //   std::cout << "lsn now: " << lsn_now << "  flushed_lsn_: " << flushed_lsn_ << "    count: " << MAX_LOG_SIZE
    //             << std::endl;
    //   std::unique_ptr<char> log(new char[MAX_LOG_SIZE]);
      char *get_log = new char[MAX_LOG_SIZE];
      disk_.ReadLog(lsn_now, MAX_LOG_SIZE, get_log);
      std::shared_ptr<LogRecord> log_record = LogRecord::DeserializeFrom(lsn_now, get_log);
      oid_t table_oid;
      pageid_t page_id;
      if (log_record->GetType() == LogType::INSERT) {
        auto insert_ptr = std::dynamic_pointer_cast<InsertLog>(log_record);
        table_oid = insert_ptr->GetOid();
        page_id = insert_ptr->GetPageId();
      } else if (log_record->GetType() == LogType::DELETE) {
        auto delete_ptr = std::dynamic_pointer_cast<DeleteLog>(log_record);
        table_oid = delete_ptr->GetOid();
        page_id = delete_ptr->GetPageId();
      } else if (log_record->GetType() == LogType::NEW_PAGE) {
        auto newpage_ptr = std::dynamic_pointer_cast<NewPageLog>(log_record);
        table_oid = newpage_ptr->GetOid();
        page_id = newpage_ptr->GetPageId();
        
      } else {
        lsn_now = lsn_now + log_record->GetSize();
        delete[] get_log;
        continue;
      }
      if (dpt_.find({table_oid, page_id}) == dpt_.end()) {
        ;
      } else {
        lsn_t rec_lsn = dpt_[{table_oid, page_id}];
        if (log_record->GetLSN() < rec_lsn) {
          ;
        } else {
            if (log_record->GetType() == LogType::NEW_PAGE){
                log_record->Redo(*buffer_pool_, *catalog_, *this);
            }else{
                auto page = buffer_pool_->GetPage(catalog_->GetDatabaseOid(table_oid), table_oid, page_id);
                auto table_page = std::make_shared<TablePage>(page);
                auto page_lsn = table_page->GetPageLSN();
                std::cout<<"lsn now: "<<lsn_now<<"    page lsn:"<<page_lsn<<std::endl;
                if (lsn_now <= page_lsn) {
                    ;
                } else {
                    log_record->Redo(*buffer_pool_, *catalog_, *this);
                }
            }
        }
      }
      delete[] get_log;
      lsn_now = lsn_now + log_record->GetSize();
    //   log_record->Redo(*buffer_pool_, * catalog_, *this);
    }
  }
  std::cout<<"finish redo"<<std::endl;
}

void LogManager::Undo() {
  // 根据活跃事务表，将所有活跃事务回滚
  // LAB 2 BEGIN
  std::cout<<"undo"<<std::endl;
  for(auto it:att_){
    Rollback(it.first);
  }
  std::cout<<"finish undo"<<std::endl;
}

}  // namespace huadb
