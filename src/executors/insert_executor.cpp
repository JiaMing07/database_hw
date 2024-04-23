#include "executors/insert_executor.h"

namespace huadb {

InsertExecutor::InsertExecutor(ExecutorContext &context, std::shared_ptr<const InsertOperator> plan,
                               std::shared_ptr<Executor> child)
    : Executor(context, {std::move(child)}), plan_(std::move(plan)) {}

void InsertExecutor::Init() {
  children_[0]->Init();
  table_ = context_.GetCatalog().GetTable(plan_->GetTableOid());
  column_list_ = context_.GetCatalog().GetTableColumnList(plan_->GetTableOid());
}

std::shared_ptr<Record> InsertExecutor::Next() {
  if (finished_) {
    return nullptr;
  }
  uint32_t count = 0;
  while (auto record = children_[0]->Next()) {
    std::vector<Value> values(column_list_.Length());
    const auto &insert_columns = plan_->GetInsertColumns().GetColumns();
    for (size_t i = 0; i < insert_columns.size(); i++) {
      auto column_index = column_list_.GetColumnIndex(insert_columns[i].GetName());
      values[column_index] = record->GetValue(i);
    }
    auto table_record = std::make_shared<Record>(std::move(values));
    // 通过 context_ 获取正确的锁，加锁失败时抛出异常
    // LAB 3 BEGIN
    std::cout<<table_record->ToString()<<std::endl;
    bool table_lock_flag = context_.GetLockManager().LockTable(context_.GetXid(), LockType::IX, plan_->GetTableOid());
    if(table_lock_flag == false){
        throw DbException("insert table lock failed!");
    }
    auto rid = table_->InsertRecord(std::move(table_record), context_.GetXid(), context_.GetCid(), true);
    bool lock_flag = context_.GetLockManager().LockRow(context_.GetXid(), LockType::X, plan_->GetTableOid(), rid);
    if(lock_flag == false){
        throw DbException("insert lock failed!");
    }
    std::cout<<"finish insert"<<std::endl;
    count++;
  }
  finished_ = true;
  return std::make_shared<Record>(std::vector{Value(count)});
}

}  // namespace huadb
