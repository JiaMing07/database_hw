#include "executors/nested_loop_join_executor.h"

namespace huadb {

NestedLoopJoinExecutor::NestedLoopJoinExecutor(ExecutorContext &context,
                                               std::shared_ptr<const NestedLoopJoinOperator> plan,
                                               std::shared_ptr<Executor> left, std::shared_ptr<Executor> right)
    : Executor(context, {std::move(left), std::move(right)}), plan_(std::move(plan)) {}

void NestedLoopJoinExecutor::Init() {
  children_[0]->Init();
  children_[1]->Init();
}

std::shared_ptr<Record> NestedLoopJoinExecutor::Next() {
  // 从 NestedLoopJoinOperator 中获取连接条件
  // 使用 OperatorExpression 的 EvaluateJoin 函数判断是否满足 join 条件
  // 使用 Record 的 Append 函数进行记录的连接
  // LAB 4 BEGIN
  if(cnt == 0){
    std::vector<std::shared_ptr<Record>> records0;
    std::vector<std::shared_ptr<Record>> records1;
    while (auto record = children_[0]->Next()) {
      records0.push_back(record);
    }
    while (auto record = children_[1]->Next()) {
      records1.push_back(record);
    }
    if(records0.size() == 0 || records1.size() == 0){
        return nullptr;
    }
    int cnt0 = 0;
    int cnt1 = 0;
    for(auto r0 : records0){
        for(auto r1 : records1){
            if(plan_->join_condition_->EvaluateJoin(r0, r1).GetValue<bool>()){
                // std::cout<<"not null"<<std::endl;
                auto r_now = *r0;
                r_now.Append(*r1);
                records.push_back(r_now);
                // std::cout<<r_now.ToString()<<std::endl;
            }
        }
    }
    cnt++;
    if(records.size() == 0){
        return nullptr;
    }
    std::shared_ptr<Record> record_ptr(new Record(records[0].GetValues(), records[0].GetRid()));
    record_ptr->SetDeleted(records[0].IsDeleted());
    record_ptr->SetCid(records[0].GetCid());
    record_ptr->SetXmax(records[0].GetXmax());
    record_ptr->SetXmin(records[0].GetXmin());
    return record_ptr;
  }
  if(cnt < records.size()){
    cnt++;
    std::shared_ptr<Record> record_ptr(new Record(records[cnt - 1].GetValues(), records[cnt - 1].GetRid()));
    record_ptr->SetDeleted(records[cnt - 1].IsDeleted());
    record_ptr->SetCid(records[cnt - 1].GetCid());
    record_ptr->SetXmax(records[cnt - 1].GetXmax());
    record_ptr->SetXmin(records[cnt - 1].GetXmin());
    return record_ptr;
  }else{
    return nullptr;
  }
  return nullptr;
}

}  // namespace huadb
