#include "transaction/lock.h"

namespace huadb {
    LockGranularity Lock::GetLockGranularity(){
        return lock_granularity_;
    }

    LockType Lock::GetLockType(){
        return lock_type_;
    }

    oid_t Lock::GetOid(){
        return oid_;
    }
    Rid Lock::GetRid(){
        return rid_;
    }

    void Lock::SetLockGranularity(LockGranularity lock_granularity){
        lock_granularity_ = lock_granularity;
    }

    void Lock::SetLockType(LockType lock_type){
        lock_type_ = lock_type;
    }

    void Lock::SetOid(oid_t oid){
        oid_ = oid;
    }
    void Lock::SetRid(Rid rid){
        rid_ = rid;
    }
}