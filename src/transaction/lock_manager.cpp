#include "transaction/lock_manager.h"

namespace huadb {
void LockManager::PrintXidLock(xid_t xid){
    auto all_locks = lock_map[xid];
    std::cout<<"xid: "<<xid<<std::endl;
    for(auto it:all_locks){
        it.print();
    }
}

bool LockManager::LockTable(xid_t xid, LockType lock_type, oid_t oid) {
  // 对数据表加锁，成功加锁返回 true，如果数据表已被其他事务加锁，且锁的类型不相容，返回 false
  // 如果本事务已经持有该数据表的锁，根据需要升级锁的类型
  // LAB 3 BEGIN
//   std::cout<<"lock table:"<<xid<<std::endl;
//   PrintXidLock(xid);
//   std::cout<<"----------------------"<<std::endl;
  bool exist_table_lock = false;
  auto table_locks = lock_map[xid];
  LockType table_lock_type_now;
  int idx = 0;
  for (auto it : table_locks) {
    if (it.lock_granularity_ == LockGranularity::TABLE && oid == it.oid_) {
      exist_table_lock = true;
    //   std::cout << "exist table lock" << std::endl;
      it.lock_type_ = Upgrade(it.lock_type_, lock_type);
      lock_map[xid][idx] = it;
      table_lock_type_now = it.lock_type_;
    }
    idx++;
  }
  if(exist_table_lock == false){
    Lock lock_table;
    lock_table.lock_granularity_ = LockGranularity::TABLE;
    lock_table.lock_type_ = lock_type;
    lock_table.oid_ = oid;
    lock_map[xid].push_back(lock_table);
    table_lock_type_now = lock_type;
  }
  for(auto iter:lock_map){
    auto all_locks = lock_map[iter.first];
    if (iter.first != xid) {
      for (auto it : all_locks) {
        if (it.lock_granularity_ == LockGranularity::TABLE && oid == it.oid_) {
          if (!Compatible(it.lock_type_, table_lock_type_now)) {
             return false;
          }
        }
      }
    }
  }
//   PrintXidLock(xid);
  return true;
}

bool LockManager::LockRow(xid_t xid, LockType lock_type, oid_t oid, Rid rid) {
  // 对数据行加锁，成功加锁返回 true，如果数据行已被其他事务加锁，且锁的类型不相容，返回 false
  // 如果本事务已经持有该数据行的锁，根据需要升级锁的类型
  // LAB 3 BEGIN
//   std::cout<<"lock row:"<<xid<<std::endl;
//   PrintXidLock(xid);
//   std::cout<<"----------------------"<<std::endl;
  bool exist_table_lock = false;
  bool exist_row_lock = false;
  auto xid_locks = lock_map[xid];
  LockType table_lock_type_now;
  LockType row_lock_type_now;
  int idx = 0;
  for (auto it : xid_locks) {
    if (it.lock_granularity_ == LockGranularity::TABLE && oid == it.oid_) {
      exist_table_lock = true;
    //   std::cout << "exist table lock" << std::endl;
    //   it.print();
      LockType upgrade_type;
      if (lock_type == LockType::S) {
        upgrade_type = LockType::IS;
      } else {
        upgrade_type = LockType::IX;
      }
    //   std::cout << "upgrade: " << (upgrade_type == LockType::IX) << std::endl;
      it.lock_type_ = Upgrade(it.lock_type_, upgrade_type);
      lock_map[xid][idx].lock_type_ = it.lock_type_;
    //   std::cout << "table lock type "<< (it.lock_type_ == LockType::IX) << std::endl;
      table_lock_type_now = it.lock_type_;
    } else if (it.lock_granularity_ == LockGranularity::ROW && it.oid_ == oid && it.rid_.page_id_ == rid.page_id_ &&
               it.rid_.slot_id_ == rid.slot_id_) {
      exist_row_lock = true;
    //   std::cout << "exist row lock" << std::endl;
    //   it.print();
    //   std::cout << (lock_type == LockType::X) << std::endl;
      it.lock_type_ = Upgrade(it.lock_type_, lock_type);
      row_lock_type_now = it.lock_type_;
    //   std::cout << (it.lock_type_ == LockType::X) << std::endl;
      lock_map[xid][idx].lock_type_ = it.lock_type_;
    //   PrintXidLock(xid);
    }
    idx++;
  }
  if(exist_row_lock == false){
    Lock lock_row = {LockGranularity::ROW, lock_type, oid, rid};
    lock_map[xid].push_back(lock_row);
    row_lock_type_now = lock_row.lock_type_;
  }
  if(exist_table_lock == false){
    Lock lock_table;
    lock_table.lock_granularity_ = LockGranularity::TABLE;
    if(lock_type == LockType::S){
        lock_table.lock_type_ = LockType::IS;
    }else{
        lock_table.lock_type_ = LockType::IX;
    }
    lock_table.oid_ = oid;
    lock_map[xid].push_back(lock_table);
    table_lock_type_now = lock_table.lock_type_;
    // std::cout<<"new table lock: "<<std::endl;
    // lock_table.print();
    // std::cout<<(lock_table.lock_type_ == LockType::IX)<<std::endl;
  }
  for (auto iter : lock_map) {
    auto all_locks = lock_map[iter.first];
    // if(all_locks.size() > 0){
    //     std::cout<<iter.first<<std::endl;
    //     // PrintXidLock(iter.first);
    //     std::cout<<"----------------------"<<std::endl;
    // }
    if (iter.first != xid) {
      for (auto it : all_locks) {
        if (it.lock_granularity_ == LockGranularity::TABLE && oid == it.oid_) {
        //   std::cout << "exist other xid table lock" << std::endl;
          if (!Compatible(it.lock_type_, table_lock_type_now)) {
            return false;
          }
        } else if (it.lock_granularity_ == LockGranularity::ROW && it.oid_ == oid && it.rid_.page_id_ == rid.page_id_ &&
                   it.rid_.slot_id_ == rid.slot_id_) {
        //   std::cout << "exist other xid row lock" << std::endl;
        //   it.print();
        //   std::cout << (lock_type == LockType::X) << "  " << (it.lock_type_ == LockType::X) << std::endl;
          if (!Compatible(it.lock_type_, row_lock_type_now)) {
            return false;
          }
        }
      }
    }
  }
//   PrintXidLock(xid);
//   std::cout<<"finish lock row"<<std::endl;
  return true;
}

void LockManager::ReleaseLocks(xid_t xid) {
  // 释放事务 xid 持有的所有锁
  // LAB 3 BEGIN
//   std::cout<<"release: "<<xid<<std::endl;
  if(!lock_map[xid].empty()){
    lock_map[xid].clear();
  }
}

void LockManager::SetDeadLockType(DeadlockType deadlock_type) { deadlock_type_ = deadlock_type; }

bool LockManager::Compatible(LockType type_a, LockType type_b) const {
  // 判断锁是否相容
  // LAB 3 BEGIN
  if(type_a == LockType::S){
    if(type_b == LockType::S || type_b == LockType::IS){
        return true;
    }else{
        return false;
    }
  }else if(type_a == LockType::X){
    return false;
  }else if(type_a == LockType::IS){
    if(type_b == LockType::X){
        return false;
    }else{
        return true;
    }
  }else if(type_a == LockType::IX){
    if(type_b == LockType::IS || type_b == LockType::IX){
        return true;
    }else{
        return false;
    }
  }else if(type_a == LockType::SIX){
    if(type_b == LockType::IS){
        return true;
    }else{
        return false;
    }
  }
  return true;
}

LockType LockManager::Upgrade(LockType self, LockType other) const {
  // 升级锁类型
  // LAB 3 BEGIN
  if(self == LockType::S){
    // std::cout<<"lock S ";
    if(other == LockType::X){
        // std::cout<<"lock X"<<std::endl;
        return LockType::X;
    }else if(other == LockType::IX){
        // std::cout<<"lock IX"<<std::endl;
        return LockType::SIX;
    }else{
        return LockType::S;
    }
  }else if(self == LockType::X){
    return LockType::X;
  }else if(self == LockType::IS){
    return other;
  }else if(self == LockType::IX){
    // std::cout<<"lock IX ";
    if(other == LockType::S){
        return LockType::SIX;
    }else if(other == LockType::X || other == LockType::SIX){
        // std::cout<<"lock X or SIX"<<std::endl;
        return other;
    }else{
        return self;
    }
  }else if(self == LockType::SIX){
    if(other == LockType::X){
        return other;
    }else{
        return self;
    }
  }
  return LockType::IS;
}

}  // namespace huadb
