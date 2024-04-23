#pragma once
// #include "common/types.h"
#include "transaction/lock_manager.h"

namespace huadb {


class Lock {
public:
    LockGranularity GetLockGranularity();
    LockType GetLockType();
    oid_t GetOid();
    Rid GetRid();

    void SetLockGranularity(LockGranularity lock_granularity);
    void SetLockType(LockType lock_type);
    void SetOid(oid_t oid);
    void SetRid(Rid rid);
private:
    LockGranularity lock_granularity_;
    LockType lock_type_;
    oid_t oid_;
    Rid rid_;
};

}  // namespace huadb