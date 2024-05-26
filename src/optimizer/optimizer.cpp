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
  std::cout<<"optimize finish"<<std::endl;
  return plan;
}

std::shared_ptr<Operator> split(std::shared_ptr<Operator> plan){
    auto children = plan->GetChildren();
    int cnt = 0;
    for(auto child:children){
      std::shared_ptr<Operator> operator_left;
      std::shared_ptr<Operator> operator_right;
      if (child->GetType() == OperatorType::FILTER) {
        // std::cout<<"split filter: "<<plan->ToString()<<std::endl;
        auto filter_child = std::dynamic_pointer_cast<FilterOperator>(child);
        if (filter_child->predicate_->GetExprType() == OperatorExpressionType::LOGIC) {
          auto logic = std::dynamic_pointer_cast<Logic>(filter_child->predicate_);
        //   std::cout<<"logic: "<<logic->ToString()<<std::endl;
          if (logic->GetLogicType() == LogicType::AND && filter_child->predicate_->children_.size() >= 2) {
            auto predicate_children = filter_child->predicate_->children_;
            // std::cout << "predicate length: " << predicate_children.size() << std::endl;
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
            // std::cout<<"left: "<<predicate_left->ToString()<<std::endl;
            // std::cout<<"right: "<<predicate_right->ToString()<<std::endl;
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
    // std::cout<<"children: "<<std::endl;
    // for(auto child:children){
    //     std::cout<<child->ToString()<<std::endl;;
    // }
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
//   std::cout<<"push down filter: "<<filter_plan->ToString()<<std::endl;
  bool connect = false;
  if(filter_plan->predicate_->GetExprType() == OperatorExpressionType::COMPARISON){
    // std::cout<<"comparison: "<<filter_plan->ToString()<<std::endl;
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
  auto proj_plan = std::dynamic_pointer_cast<ProjectionOperator>(plan);
  for(auto expr:proj_plan->exprs_){
    auto expr_ptr = std::dynamic_pointer_cast<ColumnValue>(expr);
    std::string name = expr->ToString();
    int pos = name.find('.', 0);
    std::string table_name = name.substr(0, pos);
    if(project_map.find(table_name) == project_map.end()){
        project_map[table_name] = {expr};
    }else{
        project_map[table_name].push_back(expr);
    }
  }
  proj_success = false;
  plan->children_[0] = PushDown(plan->children_[0]);
//   if(proj_success){
//     return plan->children_[0];
//   }
  return plan;
}

std::shared_ptr<Operator> Optimizer::PushDownJoin(std::shared_ptr<Operator> plan) {
  // ColumnValue 的 name_ 字段为 "table_name.column_name" 的形式
  // 判断当前查询计划树的连接谓词是否使用当前 NestedLoopJoin 节点涉及到的列
  // 如果是，将连接谓词添加到当前的 NestedLoopJoin 节点的 join_condition_ 中
  // LAB 5 BEGIN
  auto nested_loop_join_plan = std::dynamic_pointer_cast<NestedLoopJoinOperator>(plan);
//   std::cout<<"push down join left: "<<nested_loop_join_plan->children_[0]->column_list_->ToString()<<std::endl;
//   std::cout<<"push down join right: "<<nested_loop_join_plan->children_[1]->column_list_->ToString()<<std::endl;
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
        // std::cout<<"join: "<<name_1<<"  "<<name_2<<std::endl;
        if(std::find(column_names.begin(), column_names.end(), name_1) != column_names.end() && std::find(column_names.begin(), column_names.end(), name_2) != column_names.end()){
            nested_loop_join_plan->join_condition_ = filter_pred;
            success = true;
            int pos = name_1.find('.', 0);
            std::string table_name_1 = name_1.substr(0, pos);
            pos = name_2.find('.', 0);
            std::string table_name_2 = name_2.substr(0, pos);
            if(project_map.find(table_name_1) == project_map.end()){
                project_map[table_name_1] = {filter_pred->children_[0]};
            }else{
                if(find(project_map[table_name_1].begin(), project_map[table_name_1].end(), filter_pred->children_[0]) == project_map[table_name_1].end()){
                    project_map[table_name_1].push_back(filter_pred->children_[0]);
                }
            }
            if(project_map.find(table_name_2) == project_map.end()){
                project_map[table_name_2] = {filter_pred->children_[1]};
            }else{
                if(find(project_map[table_name_2].begin(), project_map[table_name_2].end(), filter_pred->children_[1]) == project_map[table_name_2].end()){
                    project_map[table_name_2].push_back(filter_pred->children_[1]);
                }
            }
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
  std::shared_ptr<Operator> res = plan;
  auto seq_scan_op = std::dynamic_pointer_cast<SeqScanOperator>(plan);
  for (int i = 0; i < filter_predicate.size(); i++) {
    auto filter_pred = filter_predicate[i];
    auto connect = is_connected[i];
    name_1 = filter_pred->children_[0]->ToString();
    // std::cout << "push down seq name: " << name_1 << "is connected" << connect << std::endl;
    // std::cout << "predicate: " << filter_pred->ToString() << std::endl;
    int pos = name_1.find('.', 0);
    std::string table_name;
    if (pos != -1) {
      table_name = name_1.substr(0, pos);
    } else {
    //   std::cout << "pos = -1: " << name_1 << std::endl;
    }
    // std::cout << "seq table name: " << seq_scan_op->GetTableNameOrAlias() << std::endl;
    if (!connect && table_name == seq_scan_op->GetTableNameOrAlias()) {
      auto filter = std::make_shared<FilterOperator>(plan->column_list_, plan, filter_pred);
      success = true;
      res = filter;
    }
  }
  if(enable_projection_pushdown_ && project_map.find(seq_scan_op->GetTableNameOrAlias()) != project_map.end()){
    auto proj = std::make_shared<ProjectionOperator>(plan->column_list_, res, project_map[seq_scan_op->GetTableNameOrAlias()]);
    res = proj;
  }
  
  return res;
}

void print_map(std::map<std::string, uint32_t> map){
    for(auto it: map){
        std::cout<<it.first<<"  "<<it.second<<std::endl;
    }
}

std::shared_ptr<Operator> Optimizer::ReorderJoin(std::shared_ptr<Operator> plan) {
  // 通过 catalog_.GetCardinality 和 catalog_.GetDistinct 从系统表中读取表和列的元信息
  // 可根据 join_order_algorithm_ 变量的值选择不同的连接顺序选择算法，默认为 None，表示不进行连接顺序优化
  // LAB 5 BEGIN
  if(join_order_algorithm_ == JoinOrderAlgorithm::GREEDY){
    auto table_names = catalog_.GetTableNames();
    std::map<std::string, uint32_t> cardinalities;
    std::map<std::string, std::map<std::string, uint32_t>> distincts;
    for(auto table_name: table_names){
        cardinalities[table_name] = catalog_.GetCardinality(table_name);
        auto columns = catalog_.GetTableColumnList(table_name);
        std::map<std::string, uint32_t> table_distincts;
        for(auto column: columns.GetColumns()){
            auto column_name = column.GetName();
            table_distincts[column_name] = catalog_.GetDistinct(table_name, column_name);
        }
        distincts[table_name] = table_distincts;
    }
    // std::cout<<"reorder join"<<std::endl;
    // print_map(cardinalities);
    // for(auto it: distincts){
    //     std::cout<<it.first<<": "<<std::endl;
    //     print_map(it.second);
    // }
    std::vector<std::shared_ptr<Operator>> plan_now = {plan};
    std::vector<std::shared_ptr<Operator>> nested_loop_join_plans;
    while(plan_now.size() > 0){
        auto children = plan_now[0]->children_;
        plan_now.erase(plan_now.begin());
        for(auto child: children){
            if(child->GetType() == OperatorType::NESTEDLOOP){
                nested_loop_join_plans.push_back(child);
            }
            plan_now.push_back(child);
        }
    }
    std::map<std::string, uint32_t> cnt;
    std::vector<std::string> tables;
    std::map<std::string, std::shared_ptr<Operator>> nodes;
    std::map<std::pair<std::string, std::string>, std::shared_ptr<OperatorExpression>> relations;
    for(auto join_plan: nested_loop_join_plans){
        auto nested_join_plan = std::dynamic_pointer_cast<NestedLoopJoinOperator>(join_plan);
        auto predicate = nested_join_plan->join_condition_;
        auto left_name = predicate->children_[0]->ToString();
        auto right_name = predicate->children_[1]->ToString();
        int pos = left_name.find('.', 0);
        auto left_table_name = left_name.substr(0, pos);
        pos = right_name.find('.', 0);
        auto right_table_name = right_name.substr(0, pos);
        relations[{left_table_name, right_table_name}] = predicate;
        auto find_left = find(tables.begin(), tables.end(), left_table_name);
        auto find_right = find(tables.begin(), tables.end(), right_table_name);
        if(find_left == table_names.end()){
            table_names.push_back(left_table_name);
            cnt[left_table_name] = 1;
        }else{
            cnt[left_table_name] += 1;
        }
        if(find_right == table_names.end()){
            table_names.push_back(right_table_name);
            cnt[right_table_name] = 1;
        }else{
            cnt[right_table_name] += 1;
        }
        if(nested_join_plan->children_[0]->GetType() != OperatorType::NESTEDLOOP){
            // nodes[left_table_name] = nested_join_plan->children_[0];
            auto columns_0 = nested_join_plan->children_[0]->OutputColumns();
            // std::cout<<"columns: "<<columns_0.ToString()<<std::endl;
            auto columns_def_0 = columns_0.GetColumns();
            auto table_col_name_0 = columns_def_0[0].GetName();
            int pos_0 = table_col_name_0.find('.', 0);
            auto table_name_0 = table_col_name_0.substr(0, pos_0);
            nodes[table_name_0] = nested_join_plan->children_[0];
        }
        if(nested_join_plan->children_[1]->GetType() != OperatorType::NESTEDLOOP){
            // nodes[right_table_name] = nested_join_plan->children_[0];
            auto columns_1 = nested_join_plan->children_[1]->OutputColumns();
            // std::cout<<"columns: "<<columns_1.ToString()<<std::endl;
            auto columns_def_1 = columns_1.GetColumns();
            auto table_col_name_1 = columns_def_1[0].GetName();
            int pos_1 = table_col_name_1.find('.', 0);
            auto table_name_1 = table_col_name_1.substr(0, pos_1);
            nodes[table_name_1] = nested_join_plan->children_[1];
        }
    }
    // std::cout<<"cnt: "<<std::endl;
    // print_map(cnt);
    // for(auto it: distincts){
    //     std::cout<<it.first<<": "<<std::endl;
    //     print_map(it.second);
    // }
    if(nested_loop_join_plans.size() == 0){
        return plan;
    }
    std::vector<std::string> r_now;
    std::vector<std::string> v_now; // r_curr 里有多少 V(S, feature)
    std::shared_ptr<Operator> child_0;
    while(nested_loop_join_plans.size() > 0){
        std::map<std::string, uint32_t> v_map;
        std::string r_curr;
        if(r_now.size() == 0){
            uint32_t min_tree = INVALID_CARDINALITY;
            uint32_t max_cnt = 0;
            for(auto it:cardinalities){
                if(it.second < min_tree || cnt[it.first] > max_cnt){
                    min_tree = it.second;
                    max_cnt = cnt[it.first];
                    r_curr = it.first;
                }
            }
            r_now.push_back(r_curr);
            for(auto i_: distincts[r_curr]){
                v_now.push_back(i_.first);
            }
            child_0 = nodes[r_curr];
        }
        if(r_now.size() > 0){
            // std::cout<<"size > 0"<<std::endl;
            for(auto it:distincts){
                if(find(r_now.begin(), r_now.end(), it.first) == r_now.end()){
                    bool flag = true;
                    std::map<std::string, uint32_t> min_feature;
                    std::map<std::string, uint32_t> mul_feature;
                    uint32_t mul_all = cardinalities[it.first];
                    if(flag){
                        std::string r_cur_nex;
                        for(auto i_:distincts[it.first]){
                            min_feature[i_.first] = i_.second;
                            mul_feature[i_.first] = i_.second;
                        }
                        for(auto r:r_now){
                            mul_all *= cardinalities[r];
                            r_cur_nex += (r + ',');
                            for(auto i_:distincts[r]){
                                if(min_feature.find(i_.first) != min_feature.end()){
                                    if(min_feature[i_.first] > i_.second){
                                        min_feature[i_.first] = i_.second;
                                    }
                                    mul_feature[i_.first] *= i_.second;
                                }else{
                                    min_feature[i_.first] = i_.second;
                                    mul_feature[i_.first] = i_.second;
                                }
                            }
                        }
                        // std::cout<<"feature: "<<std::endl;
                        // print_map(min_feature);
                        // print_map(mul_feature);
                        r_cur_nex += it.first;
                        // std::cout<<"r_cur_nex:"<<r_cur_nex<<std::endl;
                        v_map[r_cur_nex] = mul_all;
                        for(auto j: min_feature){
                            v_map[r_cur_nex] = v_map[r_cur_nex] * min_feature[j.first] / mul_feature[j.first];
                        }
                    }
                }
            }
            // std::cout<<"v_map: "<<std::endl;
            // print_map(v_map);
            int min_tree_size_now = INVALID_CARDINALITY;
            std::string r_nex;
            for(auto it:v_map){
                if(it.second < min_tree_size_now){
                    min_tree_size_now = it.second;
                    auto pos_ = it.first.rfind(',');
                    r_nex = it.first.substr(pos_+1, it.first.size());
                    // r_now.push_back(std::string(1, it.first[it.first.size() - 1]));
                }
            }
            // std::cout<<"r_nex: "<<r_nex<<"  "<<min_tree_size_now<<std::endl;;
            int count = 0;
            for(auto it:relations){
                if((it.first.first == r_nex && find(r_now.begin(), r_now.end(), it.first.second) != r_now.end()) || (it.first.second == r_nex && find(r_now.begin(), r_now.end(), it.first.first) != r_now.end())){
                    auto join_plan_now = nested_loop_join_plans[nested_loop_join_plans.size() - 1];
                    auto nested_join_plan_now = std::dynamic_pointer_cast<NestedLoopJoinOperator>(join_plan_now);
                    nested_join_plan_now->children_[0] = child_0;
                    nested_join_plan_now->children_[1] = nodes[r_nex];
                    nested_join_plan_now->join_condition_ = it.second;
                    child_0 = join_plan_now;
                    nested_loop_join_plans.pop_back();
                }
                count++;
            }
            r_now.push_back(r_nex);
        }
    }
  }else if(join_order_algorithm_ == JoinOrderAlgorithm::DP){

  }
  return plan;
}

}  // namespace huadb
