#include <iostream>
#include "psmalloc.c"
#include <unordered_map>
#include <sys/mman.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <stdio.h>

#define PAGE_MAGIC  9627
#define DEFAULT_CHUNK_NUM  5

struct pageHeader{
    short magic;
    short chunkSize;
    int freeChunkOffset;
    int pageSize;
};
pageHeader* InitPage(const char *filename, int chunkSize, int chunkNum);
int ExtendPage(const char *filename);
std:: unordered_map<const char *,intptr_t> pageFdSet;

// intptr_t alloc(const char *filename, size_t inSize) {
//     uint64_t returnPtr = 0;

//     FILE *fd;
//     if(!pageFdSet.count(filename) || pageFdSet[filename]==NULL){
//         printf("add page %s \n",filename);
//         fd = fopen (filename, "r+b");
//         pageFdSet[filename] = fd;
//     }
    
// }

void closeAllPages(){
    for(const std::pair<const char *,intptr_t> & kv:pageFdSet){
        munmap((void*)kv.second,((pageHeader*)kv.second)->pageSize);
        // close(kv.second);
    }
}


void CreatePage(const char *filename, short chunkSize){
    FILE *fp;
    fp = fopen (filename, "wb");
    if (fp == NULL) {
        return;
    }
    struct pageHeader ph = {PAGE_MAGIC,chunkSize,16};
    fwrite (&ph, 16, 1, fp);
}
int chunkSizes[] = {16,32,64,128,256,512,1024,2048};
inline int roundByte(int size) {
  for(int i=0;i<8;i++){
    if(size<chunkSizes[i]) {
        return chunkSizes[i];
    }
  }
  return size;
}
void* psmalloc(const char *filename, size_t size){
    int chunkSize = round_byte(size);
    pageHeader* phPtr;
    if(!pageFdSet.count(filename)){
        phPtr = InitPage(filename,chunkSize,DEFAULT_CHUNK_NUM);
    }
    intptr_t ptr = (intptr_t)phPtr + phPtr->freeChunkOffset;
    phPtr->freeChunkOffset = *(intptr_t*)ptr;
    return (void*)ptr;
}
int main(){
    printf("hello\n");
    // void* ptr = xxmalloc(20);
    // printf("%p\n",ptr);
    // void* ptr2 = xxmalloc(20);
    // printf("%p\n",ptr2);
    // int* x = new(ptr) int(2);
    // printf("%p\n",x);
    // printf("%d\n",*(int*)ptr);
    // alloc("test.page",20);
    struct pageHeader ph = {PAGE_MAGIC,32,32,32*5};
    printf("%lu \n",sizeof(ph));
    int* ptr = (int*)psmalloc("test.p",12);
    printf("%p\n",ptr);
    ptr[0] = 9;
    // InitPage("test.p",32,5);
    // ExtendPage("test.p");
    // closeAllPages();
    
}
unsigned char initalPointers[24] = { 96,27 };
int ExtendPage(const char *filename){
    // ftruncate(fd,pageSize);
    int fd = open (filename, O_RDWR);
    if (fd == -1) {
        printf("can not open the file %s\n",filename);
        return -1;
    }
    if(!pageFdSet.count(filename)){
        return -1;
    }

    pageHeader* phPtr = (pageHeader*)pageFdSet[filename];
    if(phPtr->magic != PAGE_MAGIC){
        printf("PAGE_MAGIC ERROR %s\n",filename);
        return -1;
    }

    int pageSize = phPtr->pageSize;
    int chunkSize = phPtr->chunkSize;

    munmap(phPtr,pageSize);
    ftruncate(fd,pageSize*2);

    phPtr = (pageHeader*)mmap(NULL, pageSize*2, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    if(phPtr == MAP_FAILED){
        perror("MAP_FAILED");
    }
    close(fd);
    phPtr->pageSize = pageSize*2;
    phPtr->freeChunkOffset = pageSize;
    intptr_t chunkCurrent = pageSize;
    intptr_t chunkNext = chunkCurrent + chunkSize;
    for (int chunk_index = 0; chunk_index < (pageSize / chunkSize) - 1; chunk_index++) {
        *(intptr_t*)(chunkCurrent+(intptr_t)phPtr) = chunkNext;
        chunkCurrent += chunkSize;
        chunkNext += chunkSize;
    }                                                                                            
    // // write the last chunk of page
    *(intptr_t*)(chunkCurrent+(intptr_t)phPtr) = 0;
    return 0;
}
pageHeader* InitPage(const char *filename, int chunkSize, int chunkNum) {
    int fd = open (filename, O_RDWR+O_CREAT,S_IREAD+S_IWRITE);
    if (fd == -1) {
        printf("can not open the file %s\n",filename);
        return nullptr;
    }
    int pageSize = chunkSize*chunkNum;
    ftruncate(fd,pageSize);
    pageHeader* ptr = (pageHeader*)mmap(NULL, pageSize, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    printf("%p\n",ptr);
    if(ptr == MAP_FAILED){
        perror("mmap");
    }
    close(fd);

    ptr->magic = PAGE_MAGIC;
    ptr->chunkSize = chunkSize;
    ptr->freeChunkOffset = chunkSize;
    ptr->pageSize = pageSize;
    // struct pageHeader ph = {pageMagic,chunkSize,16};
    // printf("magic %d\n",ptr->magic);

    intptr_t chunkCurrent = chunkSize;
    intptr_t chunkNext = chunkCurrent + chunkSize;

    for (int chunk_index = 0; chunk_index<chunkNum - 2; chunk_index++) {
        *(intptr_t*)(chunkCurrent+(intptr_t)ptr) = chunkNext;
        chunkCurrent += chunkSize;
        chunkNext += chunkSize;
    }                                                                                            
    // // write the last chunk of page
    *(intptr_t*)(chunkCurrent+(intptr_t)ptr) = 0;
    
    // munmap(ptr,pageSize);
    // pageFdSet[filename] = (intptr_t)ptr;
    return ptr;
}