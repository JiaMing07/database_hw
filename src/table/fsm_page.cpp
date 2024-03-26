#include "table/fsm_page.h"

#include <sstream>
#include<cmath>
#define leftchild(x)	(2 * (x) + 1)
#define rightchild(x)	(2 * (x) + 2)
#define parentof(x)		(((x) - 1) / 2)
namespace huadb {
FSMPage::FSMPage(std::shared_ptr<Page> page): page_(page){
  page_data_ = page->GetData();
  db_size_t offset = 0;
  length = reinterpret_cast<int *>(page_data_);
  offset += sizeof(int);
  is_leaf_ = reinterpret_cast<int *>(page_data_ + offset);
  offset += sizeof(int);
  is_root_ = reinterpret_cast<int *>(page_data_ + offset);
  offset += sizeof(int);
  children_ids = reinterpret_cast<pageid_t *>(page_data_ + offset);
  offset += 16 * sizeof(pageid_t);
  fp_nodes = reinterpret_cast<FSMPageId *>(page_data_ + offset);
}

void FSMPage::Init(int is_root, int is_leaf){
    *length = 0;
    *is_leaf_ = is_leaf;
    *is_root_ = is_root;
    for(int i = 0; i < 31; i++){
        fp_nodes[i].free_space_size_ = 0;
    }
    for(int i = 0; i < 16; i++){
        children_ids[i] = NULL_PAGE_ID;
    }
    page_->SetDirty();
}

void FSMPage::SetLeaf(int is_leaf){
    *is_leaf_ = is_leaf;
}

void FSMPage::SetRoot(int is_root){
    *is_root_ = is_root;
}

void FSMPage::SetChildren(int i, pageid_t children){
    children_ids[i] = children;
}

int FSMPage::GetLeaf(){
    return *is_leaf_;
}

int FSMPage::GetRoot(){
    return *is_root_;
}
void FSMPage::UpdateNodes(int node){
    int parent = parentof(node);
    while(parent != 0){
        int leftchild = leftchild(parent);
        int rightchild = rightchild(parent);
        std::cout<<"left: "<<leftchild<<"   right: "<<rightchild<<std::endl;
        fp_nodes[parent].free_space_size_ = fp_nodes[leftchild].free_space_size_ > fp_nodes[rightchild].free_space_size_ ? fp_nodes[leftchild].free_space_size_ : fp_nodes[rightchild].free_space_size_;
        parent = parentof(parent);
    }
    fp_nodes[0].free_space_size_ = fp_nodes[1].free_space_size_ > fp_nodes[2].free_space_size_ ? fp_nodes[1].free_space_size_ : fp_nodes[2].free_space_size_;
}

int FSMPage::SearchFromRoot(int* tmp, int cnt){
    int parent = 0;
    for(int i = cnt - 1; i >= 0; i--){
        if(tmp[i] == 0){
            parent = leftchild(parent);
        }else{
            parent = rightchild(parent);
        }
    }
    return parent;
}

int FSMPage::ChangeToBinary(pageid_t num, int*tmp){
    for(int i = 0; i < 33; i++){
        tmp[i] = 0;
    }
    pageid_t tmp_page_id = num;
    int cnt = 0;
    while(tmp_page_id != 0){
        tmp[cnt] = tmp_page_id / 2;
        tmp_page_id /= 2;
        cnt++;
    }
    return cnt;
}

pageid_t FSMPage::GetChildId(int i){
    return children_ids[i];
}

db_size_t FSMPage::GetMax(){
    return fp_nodes[0].free_space_size_;
}

FSMReturn FSMPage::InsertPage(pageid_t pageid, db_size_t page_size, int level_now){
    std::cout<<"insert: "<<pageid<<"    "<<level_now<<std::endl;
    int tmp[33]; // 用来临时存储二进制表示
    int cnt = ChangeToBinary(pageid, tmp);
    int num_now = 0;
    // std::cout<<" tmp: ";
    for(int i = cnt - 1 - level_now * 4; i > 0; i--){
        // std::cout<<tmp[i]<<" "<<i<<std::endl;
        num_now *= 2;
        num_now += tmp[i];
    }
    page_->SetDirty();
    std::cout<<"num now"<<num_now<<"    page_size: "<<page_size<<std::endl;
    if(page_size > fp_nodes[num_now].free_space_size_){
        fp_nodes[15 + num_now].free_space_size_ = page_size;
        // ToString();
        UpdateNodes(15 + num_now);
    }
    // ToString();
    if(cnt > (level_now + 1) * 4){
        ToString();
        return {NULL_PAGE_ID, level_now + 1, num_now};
    }
    fp_nodes[15 + num_now].free_space_size_ = page_size;
    // ToString();
    UpdateNodes(15 + num_now);
    if(is_leaf_) children_ids[num_now] = pageid;
    ToString();
    return {pageid, -1, num_now};
    // int tmp[33]; // 用来临时存储二进制表示
    // for(int i = 0; i < 33; i++){
    //     tmp[i] = 0;
    // }
    // pageid_t tmp_page_id = pageid;
    // int cnt = 0;
    // while(tmp_page_id != 0){
    //     tmp[cnt] = tmp_page_id / 2;
    //     tmp_page_id /= 2;
    //     cnt++;
    // }
    // if(*length >= 16 || cnt > 3){
    //     return {NULL_PAGE_ID, *levels, cnt};
    // }
    // fp_nodes[*length].pageid_ = pageid;
    // fp_nodes[*length].is_leaf_ = true;
    // fp_nodes[*length].free_space_size_ = page_size;
    // UpdateNodes(*length);
    // *length++;
    // if(*length >= pow(2, *levels)){
    //     *levels++;
    // }
    // return {pageid, *levels, cnt};
}


FSMReturn FSMPage::UpdatePage(pageid_t pageid, db_size_t new_size, int level_now){
    std::cout<<"update: "<<pageid<<std::endl;
    page_->SetDirty();
    int tmp[33]; // 用来临时存储二进制表示
    int cnt = ChangeToBinary(pageid, tmp);
    int num_now = 0;
    for(int i = cnt - 1 - level_now * 4; i > 0; i--){
        num_now *= 2;
        num_now += tmp[i];
    }
    // if(cnt > (level_now + 1) * 3){
    //     return {NULL_PAGE_ID, level_now + 1, new_size};
    // }
    fp_nodes[15 + num_now].free_space_size_ = new_size;
    UpdateNodes(15 + num_now);
    std::cout<<"update"<<std::endl;
    // ToString();
    return {pageid, level_now, 0};
}

FSMReturn FSMPage::SearchPage(int need_size){
    std::cout<<"search"<<need_size<<std::endl;
    // ToString();
    int parent = 0;
    while(parent < 31){
        if(fp_nodes[parent].free_space_size_ > need_size){
            int leftchild = leftchild(parent);
            int rightchild = rightchild(parent);
            std::cout<<"leftchild: "<<leftchild<<"  "<<fp_nodes[leftchild].free_space_size_<<std::endl;
            std::cout<<"rightchild: "<<rightchild<<"  "<<fp_nodes[rightchild].free_space_size_<<std::endl;
            if(fp_nodes[leftchild].free_space_size_ > need_size && leftchild < 31){
                parent = leftchild;
            }else if(rightchild < 31){
                parent = rightchild;
            }
            if(leftchild >= 31 && rightchild >= 31){
                std::cout<<"search parent:"<<parent<<std::endl;
                break;
            }
        }else{
            return {NULL_PAGE_ID, 0, 0};
        }
    }
    // ToString();
    std::cout<<"finish "<<parent<<" parent "<<children_ids[parent-15]<<std::endl;
    // ToString();
    return {children_ids[parent-15], 0, 0};
}

void FSMPage::ToString() const{
    std::cout << "FSMPage[" << std::endl;
    std::cout << fp_nodes[0].free_space_size_ <<std::endl;
    for(int i = 1; i < 31; i++){
        std::cout <<fp_nodes[i].free_space_size_ <<"    ";
        if(i == 2 || i == 6 || i == 14){
            std::cout<<std::endl;
        }
    }
    std::cout<<std::endl;
    for(int i = 0; i < 16; i++){
        std::cout<<children_ids[i]<<"   ";
    }
    std::cout<<std::endl;
}

}