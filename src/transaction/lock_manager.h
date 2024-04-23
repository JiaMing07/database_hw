#pragma once
#include<map>
#include <vector>
#include <iostream>
#include "common/types.h"

namespace huadb {

enum class LockType {
  IS,   // 意向共享锁
  IX,   // 意向互斥锁
  S,    // 共享锁
  SIX,  // 共享意向互斥锁
  X,    // 互斥锁
};

enum class LockGranularity { TABLE, ROW };

// 高级功能：死锁预防/检测类型
enum class DeadlockType { NONE, WAIT_DIE, WOUND_WAIT, DETECTION };

struct Lock {
    LockGranularity lock_granularity_;
    LockType lock_type_;
    oid_t oid_;
    Rid rid_;
    void print(){
        if(lock_granularity_ == LockGranularity::TABLE){
            std::cout<<"lock granularity: table lock type: ";
            if(lock_type_ == LockType::S) std::cout<<"S ";
            else if(lock_type_ == LockType::X) std::cout<<"X    ";
            else if(lock_type_ == LockType::IS) std::cout<<"IS  ";
            else if(lock_type_ == LockType::IX) std::cout<<"IX  ";
            else std::cout<<"SIX    ";
            std::cout<<"oid: "<<oid_<<std::endl;;
        }else if(lock_granularity_ == LockGranularity::ROW){
            std::cout<<"lock granularity: row lock type: ";
            if(lock_type_ == LockType::S) std::cout<<"S ";
            else if(lock_type_ == LockType::X) std::cout<<"X    ";
            else if(lock_type_ == LockType::IS) std::cout<<"IS  ";
            else if(lock_type_ == LockType::IX) std::cout<<"IX  ";
            else std::cout<<"SIX    ";
            std::cout<<"oid: "<<oid_;
            std::cout<<"    rid: "<<rid_.page_id_<<"    "<<rid_.slot_id_<<std::endl;
        }
    }
};

class LockManager {
 public:
  // 获取表级锁
  bool LockTable(xid_t xid, LockType lock_type, oid_t oid);
  // 获取行级锁
  bool LockRow(xid_t xid, LockType lock_type, oid_t oid, Rid rid);

  // 释放事务申请的全部锁
  void ReleaseLocks(xid_t xid);

  void SetDeadLockType(DeadlockType deadlock_type);

  void PrintXidLock(xid_t xid);

 private:
  // 判断锁的相容性
  bool Compatible(LockType type_a, LockType type_b) const;
  // 实现锁的升级，如共享锁升级为互斥锁，输入两种锁的类型，返回升级后的锁类型
  LockType Upgrade(LockType self, LockType other) const;

  DeadlockType deadlock_type_ = DeadlockType::NONE;

  std::map<xid_t, std::vector<Lock>> lock_map;
};

}  // namespace huadb
