#include "executors/lock_rows_executor.h"

namespace huadb {

LockRowsExecutor::LockRowsExecutor(ExecutorContext &context, std::shared_ptr<const LockRowsOperator> plan,
                                   std::shared_ptr<Executor> child)
    : Executor(context, {std::move(child)}), plan_(std::move(plan)) {}

void LockRowsExecutor::Init() { children_[0]->Init(); }

std::shared_ptr<Record> LockRowsExecutor::Next() {
  auto record = children_[0]->Next();
  if (record == nullptr) {
    return nullptr;
  }
  // 根据 plan_ 的 lock type 获取正确的锁，加锁失败时抛出异常
  // LAB 3 BEGIN
  bool lock_flag = true;
  if(plan_->GetLockType() == SelectLockType::SHARE){
    std::cout<<"select for share: "<<record->GetRid().page_id_<<"   "<<record->GetRid().slot_id_<<std::endl;
    lock_flag = context_.GetLockManager().LockRow(context_.GetXid(), LockType::S, plan_->GetOid(), record->GetRid());
  }else if(plan_->GetLockType() == SelectLockType::UPDATE){
    std::cout<<"select for update: "<<record->GetRid().page_id_<<"   "<<record->GetRid().slot_id_<<std::endl;
    lock_flag = context_.GetLockManager().LockRow(context_.GetXid(), LockType::X, plan_->GetOid(), record->GetRid());
  }
  if(lock_flag == false){
    throw DbException("lock row failed!");
  }
  std::cout<<"finish select for"<<std::endl;
  return record;
}

}  // namespace huadb
