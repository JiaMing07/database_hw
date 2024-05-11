#include "executors/limit_executor.h"

namespace huadb {

LimitExecutor::LimitExecutor(ExecutorContext &context, std::shared_ptr<const LimitOperator> plan,
                             std::shared_ptr<Executor> child)
    : Executor(context, {std::move(child)}), plan_(std::move(plan)) {
        count = 0;
        offset = 0;
    }

void LimitExecutor::Init() { children_[0]->Init(); }

std::shared_ptr<Record> LimitExecutor::Next() {
  // 通过 plan_ 获取 limit 语句中的 offset 和 limit 值
  // LAB 4 BEGIN
  while (offset < plan_->limit_offset_.value_or(0)) {
    offset++;
    children_[0]->Next();
  }
  if (plan_->limit_count_.has_value()) {
    if (count < plan_->limit_count_.value()) {
        count += 1;
        return children_[0]->Next();
    } else {
        return nullptr;
    }
  }
  
  return children_[0]->Next();
}

}  // namespace huadb
