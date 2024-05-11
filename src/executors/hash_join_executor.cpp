#include "executors/hash_join_executor.h"

namespace huadb {

HashJoinExecutor::HashJoinExecutor(ExecutorContext &context, std::shared_ptr<const HashJoinOperator> plan,
                                   std::shared_ptr<Executor> left, std::shared_ptr<Executor> right)
    : Executor(context, {std::move(left), std::move(right)}), plan_(std::move(plan)) {}

void HashJoinExecutor::Init() {
  children_[0]->Init();
  children_[1]->Init();
  // LAB 4 ADVANCED BEGIN
}

void HashJoinExecutor::nestedjoin(std::vector<std::shared_ptr<Record>> records0, std::vector<std::shared_ptr<Record>> records1){
    if(records0.size() == 0 || records1.size() == 0){
        return;
    }
    int cnt0 = 0;
    int cnt1 = 0;
    std::vector<bool> right_flag(records1.size(), false);
    for(auto r0 : records0){
        bool left_flag = false;
        int idx = 0;
        for(auto r1 : records1){
            if(plan_->left_key_->Evaluate(r0).Equal(plan_->right_key_->Evaluate(r1))){
                // std::cout<<"not null"<<std::endl;
                auto r_now = *r0;
                r_now.Append(*r1);
                records.push_back(r_now);
                right_flag[idx] = true;
                left_flag = true;
                // std::cout<<r_now.ToString()<<std::endl;
            }
            idx++;
        }
        if(left_flag == false && (plan_->join_type_ == JoinType::LEFT || plan_->join_type_ == JoinType::FULL) ){
            auto rl_tmp = records1[0];
            auto rl_values = rl_tmp->GetValues();
            std::vector<Value> vec;
            for(auto v:rl_values){
                vec.push_back(Value(v.GetType(), v.GetSize()));
            }
            Record r_l = Record(vec);
            r0->Append(r_l);
            records.push_back(*r0);
        }
    }
    if(plan_->join_type_ == JoinType::RIGHT || plan_->join_type_ == JoinType::FULL){
      for (int k = 0; k < records1.size(); k++) {
        if (right_flag[k] == false) {
            auto rr_tmp = records0[0];
            auto rr_values = rr_tmp->GetValues();
            std::vector<Value> vec;
            for(auto v:rr_values){
                vec.push_back(Value(v.GetType(), v.GetSize()));
            }
            Record r_r = Record(vec);
            r_r.Append(*(records1[k]));
            records.push_back(r_r);
        }
      }
    }
}

std::shared_ptr<Record> HashJoinExecutor::Next() {
  // LAB 4 ADVANCED BEGIN
  std::cout<<"hash executor"<<std::endl;
  return nullptr;
}

}  // namespace huadb
