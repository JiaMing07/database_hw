#include "executors/merge_join_executor.h"

namespace huadb {

MergeJoinExecutor::MergeJoinExecutor(ExecutorContext &context, std::shared_ptr<const MergeJoinOperator> plan,
                                     std::shared_ptr<Executor> left, std::shared_ptr<Executor> right)
    : Executor(context, {std::move(left), std::move(right)}), plan_(std::move(plan)) {}

void MergeJoinExecutor::Init() {
  children_[0]->Init();
  children_[1]->Init();
}

std::shared_ptr<Record> MergeJoinExecutor::Next() {
  // LAB 4 BEGIN
  if (cnt == 0) {
    std::vector<std::shared_ptr<Record>> records0;
    std::vector<std::shared_ptr<Record>> records1;
    while (auto record = children_[0]->Next()) {
      records0.push_back(record);
    }
    while (auto record = children_[1]->Next()) {
      records1.push_back(record);
    }
    auto r = records0.begin();
    auto s = records1.begin();
    while (r != records0.end() && s != records1.end()) {
      while (s != records1.end() && plan_->left_key_->Evaluate(*r).Greater(plan_->right_key_->Evaluate(*s))) {
        s++;
      }
      while (r != records0.end() && plan_->left_key_->Evaluate(*r).Less(plan_->right_key_->Evaluate(*s))) {
        r++;
      }
      std::vector<std::shared_ptr<huadb::Record>>::iterator s_;
      while (s != records1.end() && r != records0.end() && plan_->left_key_->Evaluate(*r).Equal(plan_->right_key_->Evaluate(*s))) {
        s_ = s;
        while (s_ != records1.end() && r != records0.end() && plan_->left_key_->Evaluate(*r).Equal(plan_->right_key_->Evaluate(*s_))) {
          auto r_now = *(*r);
          r_now.Append(*(*s_));
          records.push_back(r_now);
          std::cout<<r_now.ToString()<<std::endl;
          s_++;
        }
        r++;
      }
      s = s_;
    }
    if(records.size() == 0){
        return nullptr;
    }
    cnt++;
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
