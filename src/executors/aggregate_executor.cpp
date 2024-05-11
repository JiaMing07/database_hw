#include "executors/aggregate_executor.h"

namespace huadb {

AggregateExecutor::AggregateExecutor(ExecutorContext &context, std::shared_ptr<const AggregateOperator> plan,
                                     std::shared_ptr<Executor> child)
    : Executor(context, {std::move(child)}), plan_(std::move(plan)) {
  // LAB 4 ADVANCED BEGIN
}

void AggregateExecutor::Init() {
  // LAB 4 ADVANCED BEGIN
  children_[0]->Init();
}

std::shared_ptr<Record> AggregateExecutor::Next() {
    std::cout<<"aggregate"<<std::endl;
  // LAB 4 ADVANCED BEGIN
  return nullptr;
}

}  // namespace huadb
