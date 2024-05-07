#include "executors/orderby_executor.h"
#include <vector>
#include <algorithm>
namespace huadb {

OrderByExecutor::OrderByExecutor(ExecutorContext &context, std::shared_ptr<const OrderByOperator> plan,
                                 std::shared_ptr<Executor> child)
    : Executor(context, {std::move(child)}), plan_(std::move(plan)) {}

void OrderByExecutor::Init() { children_[0]->Init(); }

// bool cmp(std::shared_ptr<Record> a, std::shared_ptr<Record> b){

// }

std::shared_ptr<Record> OrderByExecutor::Next() {
  // 可以使用 STL 的 sort 函数
  // 通过 OperatorExpression 的 Evaluate 函数获取 Value 的值
  // 通过 Value 的 Less, Equal, Greater 函数比较 Value 的值
  // LAB 4 BEGIN
  if(cnt == 0){
    while (auto record = children_[0]->Next()) {
      records.push_back(record);
    }
    if(records.size() == 0){
        return nullptr;
    }
    for (auto it = plan_->order_bys_.rbegin(); it != plan_->order_bys_.rend(); it++) {
      auto cmp = [it](std::shared_ptr<Record> a, std::shared_ptr<Record> b) -> bool {
        if (it->first == OrderByType::DEFAULT) {
          return it->second->Evaluate(a).Less(it->second->Evaluate(b));
        } else if (it->first == OrderByType::ASC) {
          return it->second->Evaluate(a).Less(it->second->Evaluate(b));
        } else {
          return it->second->Evaluate(a).Greater(it->second->Evaluate(b));
        }
      };
      std::sort(records.begin(), records.end(), cmp);
    }
    cnt++;
    return records[0];
  }else{
    if(cnt < records.size()){
        cnt++;
        return records[cnt - 1];
    }else{
        return nullptr;
    }
  }
  return children_[0]->Next();
}

}  // namespace huadb
