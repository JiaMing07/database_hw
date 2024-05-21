#include "optimizer/optimizer.h"
// #include "operators/filter_operator.h"
// #include "operators/seqscan_operator.h"
#include "operators/operators.h"
#include "operators/expressions/logic.h"
namespace huadb {

Optimizer::Optimizer(Catalog &catalog, JoinOrderAlgorithm join_order_algorithm, bool enable_projection_pushdown)
    : catalog_(catalog),
      join_order_algorithm_(join_order_algorithm),
      enable_projection_pushdown_(enable_projection_pushdown) {}

std::shared_ptr<Operator> Optimizer::Optimize(std::shared_ptr<Operator> plan) {
  plan = SplitPredicates(plan);
  plan = PushDown(plan);
  plan = ReorderJoin(plan);
  return plan;
}

std::shared_ptr<Operator> split(std::shared_ptr<Operator> plan){
    auto children = plan->GetChildren();
    int cnt = 0;
    for(auto child:children){
      std::shared_ptr<Operator> operator_left;
      std::shared_ptr<Operator> operator_right;
      if (child->GetType() == OperatorType::FILTER) {
        std::cout<<"split filter: "<<plan->ToString()<<std::endl;
        auto filter_child = std::dynamic_pointer_cast<FilterOperator>(child);
        if (filter_child->predicate_->GetExprType() == OperatorExpressionType::LOGIC) {
          auto logic = std::dynamic_pointer_cast<Logic>(filter_child->predicate_);
          std::cout<<"logic: "<<logic->ToString()<<std::endl;
          if (logic->GetLogicType() == LogicType::AND && filter_child->predicate_->children_.size() >= 2) {
            auto predicate_children = filter_child->predicate_->children_;
            std::cout << "predicate length: " << predicate_children.size() << std::endl;
            std::vector<std::shared_ptr<OperatorExpression>> predicate_1 =
                std::vector<std::shared_ptr<OperatorExpression>>(1, predicate_children[predicate_children.size() - 1]);
            predicate_children.pop_back();
            auto predicate_right = predicate_1[0];
            auto predicate_left = predicate_children[0];
            // auto predicate_left = std::make_shared<OperatorExpression>(
            //     predicate_1[0]->GetExprType(), predicate_1, filter_child->predicate_->GetValueType());
            OperatorExpressionType expr_type;
            if(predicate_children.size() == 1){
                expr_type = predicate_children[0]->GetExprType();
            }else{
                expr_type = OperatorExpressionType::LOGIC;
            }
            // auto predicate_right = std::make_shared<OperatorExpression>(
            //     expr_type, predicate_children, filter_child->predicate_->GetValueType());
            std::cout<<"left: "<<predicate_left->ToString()<<std::endl;
            std::cout<<"right: "<<predicate_right->ToString()<<std::endl;
            auto child_left =
                std::make_shared<FilterOperator>(filter_child->column_list_, filter_child->children_[0], predicate_left);
            auto child_right =
                std::make_shared<FilterOperator>(filter_child->column_list_, child_left, predicate_right);
            operator_left = std::dynamic_pointer_cast<Operator>(child_left);
            operator_right = std::dynamic_pointer_cast<Operator>(child_right);
            child = operator_right;
          }
        }
      }
    //   child = operator_left;
      child = split(child);
      children[cnt] = child;
      cnt++;
    }
    std::cout<<"children: "<<std::endl;
    for(auto child:children){
        std::cout<<child->ToString()<<std::endl;;
    }
    plan->children_ = children;
    return plan;
}

std::shared_ptr<Operator> Optimizer::SplitPredicates(std::shared_ptr<Operator> plan) {
  // 分解复合的选择谓词
  // 遍历查询计划树，判断每个节点是否为 Filter 节点
  // 判断 Filter 节点的谓词是否为逻辑表达式 (GetExprType() 是否为 OperatorExpressionType::LOGIC)
  // 以及逻辑表达式是否为 AND 类型 (GetLogicType() 是否为 LogicType::AND)
  // 如果是，将谓词的左右子表达式作为新的 Filter 节点添加到查询计划树中
  // LAB 5 BEGIN
  auto children = plan->GetChildren();
  std::vector<std::shared_ptr<Operator>> children_now;
  plan = split(plan);
  return plan;
}

std::shared_ptr<Operator> Optimizer::PushDown(std::shared_ptr<Operator> plan) {
  switch (plan->GetType()) {
    case OperatorType::FILTER:
      return PushDownFilter(std::move(plan));
    case OperatorType::PROJECTION:
      return PushDownProjection(std::move(plan));
    case OperatorType::NESTEDLOOP:
      return PushDownJoin(std::move(plan));
    case OperatorType::SEQSCAN:
      return PushDownSeqScan(std::move(plan));
    default: {
      for (auto &child : plan->children_) {
        child = SplitPredicates(child);
      }
      return plan;
    }
  }
}

std::shared_ptr<Operator> Optimizer::PushDownFilter(std::shared_ptr<Operator> plan) {
  // 将 plan 转为 FilterOperator 类型
  // 判断谓词（FilterOperator 的 predicate_ 字段）是否为 Comparison 类型，如果是，判断是否为 ColumnValue 和 ColumnValue
  // 的比较 若是，则该谓词为连接谓词；若不是，则该谓词为普通谓词
  // 可以将连接谓词和普通谓词存储到成员变量中，在遍历下层节点（SeqScan 和 NestedLoopJoin）时使用
  // 遍历结束后，根据谓词是否被成功下推（可以在 PushDownSeqScan 中记录），来决定 Filter
  // 节点是否还需在查询计划树种的原位置保留 若成功下推，则无需保留，通过 return plan->children_[0] 来删除节点
  // 否则，直接 return plan，保留节点
  // LAB 5 BEGIN
  auto filter_plan = std::dynamic_pointer_cast<FilterOperator>(plan);
  std::cout<<"push down filter: "<<filter_plan->ToString()<<std::endl;
  bool connect = false;
  if(filter_plan->predicate_->GetExprType() == OperatorExpressionType::COMPARISON){
    std::cout<<"comparison: "<<filter_plan->ToString()<<std::endl;
    auto predicate_children = filter_plan->predicate_->children_;
    if(predicate_children[0]->GetExprType() == OperatorExpressionType::COLUMN_VALUE && predicate_children[1]->GetExprType() == OperatorExpressionType::COLUMN_VALUE){
        connect = true;
        name_2 = predicate_children[1]->ToString();
    }else{
        connect = false;
    }
    filter_predicate.push_back(filter_plan->predicate_);
  }
  is_connected.push_back(connect);
  success = false;
  plan->children_[0] = PushDown(plan->children_[0]);
  if(success){
    return plan->children_[0];
  }
  return plan;
}

std::shared_ptr<Operator> Optimizer::PushDownProjection(std::shared_ptr<Operator> plan) {
  // LAB 5 ADVANCED BEGIN
  plan->children_[0] = PushDown(plan->children_[0]);
  return plan;
}

std::shared_ptr<Operator> Optimizer::PushDownJoin(std::shared_ptr<Operator> plan) {
  // ColumnValue 的 name_ 字段为 "table_name.column_name" 的形式
  // 判断当前查询计划树的连接谓词是否使用当前 NestedLoopJoin 节点涉及到的列
  // 如果是，将连接谓词添加到当前的 NestedLoopJoin 节点的 join_condition_ 中
  // LAB 5 BEGIN
  auto nested_loop_join_plan = std::dynamic_pointer_cast<NestedLoopJoinOperator>(plan);
  std::cout<<"push down join: "<<nested_loop_join_plan->column_list_->ToString()<<std::endl;
  std::shared_ptr<ColumnList> column_list = nested_loop_join_plan->column_list_;
  auto column_defs = column_list->GetColumns();
  std::vector<std::string> column_names;
  for(auto column_def:column_defs){
    column_names.push_back(column_def.GetName());
  }
  for(int i = 0; i < filter_predicate.size(); i++){
    auto filter_pred = filter_predicate[i];
    auto connect = is_connected[i];
    name_1 = filter_pred->children_[0]->ToString();
    name_2 = filter_pred->children_[1]->ToString();
    if(connect){
        std::cout<<"join: "<<name_1<<"  "<<name_2<<std::endl;
        if(std::find(column_names.begin(), column_names.end(), name_1) != column_names.end() && std::find(column_names.begin(), column_names.end(), name_2) != column_names.end()){
            nested_loop_join_plan->join_condition_ = filter_pred;
            success = true;
            break;
        }
    }
  }
  for (auto &child : plan->children_) {
    child = PushDown(child);
  }
  return plan;
}

std::shared_ptr<Operator> Optimizer::PushDownSeqScan(std::shared_ptr<Operator> plan) {
  // ColumnValue 的 name_ 字段为 "table_name.column_name" 的形式
  // 根据 table_name 与 SeqScanOperator 的 GetTableNameOrAlias 判断谓词是否匹配当前 SeqScan 节点的表
  // 如果匹配，在此扫描节点上方添加 Filter 节点，并将其作为返回值
  // LAB 5 BEGIN
//   ColumnValue;
  for (int i = 0; i < filter_predicate.size(); i++) {
    auto filter_pred = filter_predicate[i];
    auto connect = is_connected[i];
    name_1 = filter_pred->children_[0]->ToString();
    std::cout << "push down seq name: " << name_1 << "is connected" << connect << std::endl;
    std::cout << "predicate: " << filter_pred->ToString() << std::endl;
    int pos = name_1.find('.', 0);
    std::string table_name;
    if (pos != -1) {
      table_name = name_1.substr(0, pos);
    } else {
      std::cout << "pos = -1: " << name_1 << std::endl;
    }
    auto seq_scan_op = std::dynamic_pointer_cast<SeqScanOperator>(plan);
    std::cout << "seq table name: " << seq_scan_op->GetTableNameOrAlias() << std::endl;
    if (!connect && table_name == seq_scan_op->GetTableNameOrAlias()) {
      auto filter = std::make_shared<FilterOperator>(plan->column_list_, plan, filter_pred);
      success = true;
      return filter;
    }
  }
  return plan;
}

std::shared_ptr<Operator> Optimizer::ReorderJoin(std::shared_ptr<Operator> plan) {
  // 通过 catalog_.GetCardinality 和 catalog_.GetDistinct 从系统表中读取表和列的元信息
  // 可根据 join_order_algorithm_ 变量的值选择不同的连接顺序选择算法，默认为 None，表示不进行连接顺序优化
  // LAB 5 BEGIN
  return plan;
}

}  // namespace huadb
