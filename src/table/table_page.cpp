#include "table/table_page.h"

#include <sstream>

namespace huadb {

TablePage::TablePage(std::shared_ptr<Page> page) : page_(page) {
  page_data_ = page->GetData();
  db_size_t offset = 0;
  page_lsn_ = reinterpret_cast<lsn_t *>(page_data_);
  offset += sizeof(lsn_t);
  next_page_id_ = reinterpret_cast<pageid_t *>(page_data_ + offset);
  offset += sizeof(pageid_t);
  lower_ = reinterpret_cast<db_size_t *>(page_data_ + offset);
  offset += sizeof(db_size_t);
  upper_ = reinterpret_cast<db_size_t *>(page_data_ + offset);
  offset += sizeof(db_size_t);
  assert(offset == PAGE_HEADER_SIZE);
  slots_ = reinterpret_cast<Slot *>(page_data_ + PAGE_HEADER_SIZE);
//   std::cout<<"slots init: "<<slots_<<std::endl;
}

void TablePage::Init() {
  *page_lsn_ = 0;
  *next_page_id_ = NULL_PAGE_ID;
  *lower_ = PAGE_HEADER_SIZE;
  *upper_ = DB_PAGE_SIZE;
  page_->SetDirty();
//   std::cout<<"page init"<<*lower_<<" upper: "<<*upper_<<"   slots: "<<slots_<<std::endl;
}

slotid_t TablePage::InsertRecord(std::shared_ptr<Record> record, xid_t xid, cid_t cid) {
  // 在记录头添加事务信息（xid 和 cid）
  // LAB 3 BEGIN

  // 维护 lower 和 upper 指针
  // 设置 slots 数组
  // 将 record 写入 page data
  // 将 page 标记为 dirty
  // 返回插入的 slot id
  // LAB 1 BEGIN
  auto get_count = GetRecordCount();
  *lower_ += sizeof(Slot);
  *upper_ -= record->GetSize();
  record->SetCid(cid);
  record->SetXmin(xid);
  slots_[get_count].offset_ = *upper_;
  slots_[get_count].size_ = record->GetSize();
  record->SerializeTo(page_data_ + *upper_);
  slotid_t slots_cnt = GetRecordCount() - 1;
//   std::cout<<ToString()<<std::endl;
  page_->SetDirty();
  return GetRecordCount() - 1;
}

void TablePage::DeleteRecord(slotid_t slot_id, xid_t xid) {
  // 更改实验 1 的实现，改为通过 xid 标记删除
  // LAB 3 BEGIN

  // 将 slot_id 对应的 record 标记为删除
  // 可使用 Record::DeserializeHeaderFrom 函数读取记录头
  // 将 page 标记为 dirty
  // LAB 1 BEGIN
  RecordHeader record_header;
  db_size_t offset = slots_[slot_id].offset_;
  record_header.DeserializeFrom(page_data_ + offset);
  record_header.SetDeleted(true);
  record_header.SetXmax(xid);
  record_header.SerializeTo(page_data_ + offset);
//   RecordHeader new_header;
//   bool* is_deleted = (bool*)(page_data_ + offset);
  page_->SetDirty();
}

void TablePage::UpdateRecordInPlace(const Record &record, slotid_t slot_id) {
  record.SerializeTo(page_data_ + slots_[slot_id].offset_);
  page_->SetDirty();
}

std::shared_ptr<Record> TablePage::GetRecord(Rid rid, const ColumnList &column_list) {
  // 根据 slot_id 获取 record
  // 新建 record 并设置 rid
  // LAB 1 BEGIN
  Record record;
  db_size_t offset = slots_[rid.slot_id_].offset_;
  record.DeserializeFrom(page_data_ + offset, column_list);
  record.SetRid(rid);
  RecordHeader new_header;
  new_header.DeserializeFrom(page_data_ + offset);
  std::shared_ptr<Record> record_ptr(new Record(record.GetValues(), record.GetRid()));
  record_ptr->SetDeleted(record.IsDeleted());
  record_ptr->SetCid(record.GetCid());
  record_ptr->SetXmax(record.GetXmax());
  record_ptr->SetXmin(record.GetXmin());
//   std::shared_ptr<Record> record_ptr;
//   *record_ptr = record;
  return record_ptr;
}

void TablePage::UndoDeleteRecord(slotid_t slot_id) {
  // 修改 undo delete 的逻辑
  // LAB 3 BEGIN

  // 清除记录的删除标记
  // 将页面设为 dirty
  // LAB 2 BEGIN
  RecordHeader record_header;
  db_size_t offset = slots_[slot_id].offset_;
  record_header.DeserializeFrom(page_data_ + offset);
  record_header.SetDeleted(false);
  record_header.SetXmax(NULL_XID);
  record_header.SerializeTo(page_data_ + offset);
  page_->SetDirty();
}

void TablePage::RedoInsertRecord(slotid_t slot_id, char *raw_record, db_size_t page_offset, db_size_t record_size) {
  // 将 raw_record 写入 page data
  // 注意维护 lower 和 upper 指针，以及 slots 数组
  // 将页面设为 dirty
  // LAB 2 BEGIN
  auto get_count = GetRecordCount();
  *lower_ += sizeof(Slot);
  *upper_ -= record_size;
//   std::cout<<"lower: "<<*lower_<<"  upper: "<<*upper_<<std::endl;
//   std::cout<<"GetRecordCount before: "<<GetRecordCount()<<" slot id: "<<slot_id<<std::endl;
//   std::cout<<"slots: "<<slots_<<std::endl;
  slots_[slot_id].size_ = record_size;
  slots_[slot_id].offset_ = *upper_;
//   std::cout<<"page_data: "<<page_data_ + *upper_;
//   std::cout<<"  raw: "<<raw_record<<std::endl;
  memcpy(page_data_ + *upper_, raw_record, record_size);
//   std::cout<<ToString()<<std::endl;
  page_->SetDirty();
}

db_size_t TablePage::GetRecordCount() const { return (*lower_ - PAGE_HEADER_SIZE) / sizeof(Slot); }

lsn_t TablePage::GetPageLSN() const { return *page_lsn_; }

pageid_t TablePage::GetNextPageId() const { return *next_page_id_; }

db_size_t TablePage::GetLower() const { return *lower_; }

db_size_t TablePage::GetUpper() const { return *upper_; }

db_size_t TablePage::GetFreeSpaceSize() const {
  if (*upper_ < *lower_ + sizeof(Slot)) {
    return 0;
  } else {
    return *upper_ - *lower_ - sizeof(Slot);
  }
}

void TablePage::SetNextPageId(pageid_t page_id) {
  *next_page_id_ = page_id;
  page_->SetDirty();
}

void TablePage::SetPageLSN(lsn_t page_lsn) {
  *page_lsn_ = page_lsn;
  page_->SetDirty();
}

std::string TablePage::ToString() const {
  std::ostringstream oss;
  oss << "TablePage[" << std::endl;
  oss << "  page_lsn: " << *page_lsn_ << std::endl;
  oss << "  next_page_id: " << *next_page_id_ << std::endl;
  oss << "  lower: " << *lower_ << std::endl;
  oss << "  upper: " << *upper_ << std::endl;
  if (*lower_ > *upper_) {
    oss << "\n***Error: lower > upper***" << std::endl;
  }
  oss << "  slots: " << std::endl;
  for (size_t i = 0; i < GetRecordCount(); i++) {
    oss << "    " << i << ": offset " << slots_[i].offset_ << ", size " << slots_[i].size_ << " ";
    if (slots_[i].size_ <= RECORD_HEADER_SIZE) {
      oss << "***Error: record size smaller than header size***" << std::endl;
    } else if (slots_[i].offset_ + RECORD_HEADER_SIZE >= DB_PAGE_SIZE) {
      oss << "***Error: record offset out of page boundary***" << std::endl;
    } else {
      RecordHeader header;
      header.DeserializeFrom(page_data_ + slots_[i].offset_);
      oss << header.ToString() << std::endl;
    }
  }
  oss << "]\n";
  return oss.str();
}

}  // namespace huadb
