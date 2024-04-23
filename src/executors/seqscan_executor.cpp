#include "executors/seqscan_executor.h"

namespace huadb {

SeqScanExecutor::SeqScanExecutor(ExecutorContext &context, std::shared_ptr<const SeqScanOperator> plan)
    : Executor(context, {}), plan_(std::move(plan)) {}

void SeqScanExecutor::Init() {
  auto table = context_.GetCatalog().GetTable(plan_->GetTableOid());
  scan_ = std::make_unique<TableScan>(context_.GetBufferPool(), table, Rid{table->GetFirstPageId(), 0});
}

std::shared_ptr<Record> SeqScanExecutor::Next() {
  std::unordered_set<xid_t> active_xids;
  // 根据隔离级别，获取活跃事务的 xid（通过 context_ 获取需要的信息）
  // 通过 context_ 获取正确的锁，加锁失败时抛出异常
  // LAB 3 BEGIN
  if(context_.GetIsolationLevel() == IsolationLevel::REPEATABLE_READ){
    active_xids = context_.GetTransactionManager().GetSnapshot(context_.GetXid());
    context_.GetTransactionManager().GetActiveTransactions();
  }else if(context_.GetIsolationLevel() == IsolationLevel::READ_COMMITTED){
    // std::cout<<"read committed"<<std::endl;
    active_xids = context_.GetTransactionManager().GetActiveTransactions();
    // for(auto it:active_xids){
    //     std::cout<<it<<"    ";
    // }
    // std::cout<<std::endl;
  }else{
    active_xids = context_.GetTransactionManager().GetActiveTransactions();
  }
//   active_xids = context_.GetTransactionManager().GetSnapshot(context_.GetXid());
  bool lock_flag = context_.GetLockManager().LockTable(context_.GetXid(), LockType::IS, plan_->GetTableOid());
  if(lock_flag == false){
    throw DbException("seq scan failed");
  }
  auto record = scan_->GetNextRecord(context_.GetXid(), context_.GetIsolationLevel(), context_.GetCid(), active_xids);
//   bool lock_flag = context_.GetLockManager().LockRow(context_.GetXid(), LockType::S, plan_->GetTableOid(), record->GetRid());
  return record;
}

}  // namespace huadb
