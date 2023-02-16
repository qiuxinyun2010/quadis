// #include <psmalloc.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <sys/mman.h> 
#include <unistd.h>
#include <string.h>
#include <psmallocv2.h>
bool write_to_page(){
    const char* filepath = "good.pg.0";
    int fd = open (filepath, O_RDWR+O_CREAT,S_IREAD+S_IWRITE);
    if (fd == -1) {
        printf("can not open the file %s\n",filepath);
        return false;
    }
    int page_size = 128;
    ftruncate(fd,page_size);
    void* ptr = mmap(NULL, page_size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    // printf("%p\n",ptr);
    if(ptr== MAP_FAILED){
        perror("mmap");
        close(fd);
        return false;
    }
    close(fd);
    char * s = "我爱你";
    memcpy(ptr,s,11);
    munmap(ptr, page_size);
    return true;
}

void read_page(){
    FILE* fp;

    fp = fopen("test.entry" , "r");
    if(fp == NULL) {
        perror("打开文件时发生错误");
        return;
    }
    // int len = 0;
    int i = 0;

    // 1234\r\n\0 7
    char current_line[100] = {0};
    while (fgets(current_line, sizeof(current_line), fp) != NULL)
    {
        current_line[strlen(current_line)-1] = '\0';
    	printf("%s-%d\n",current_line,strlen(current_line));
    }
    fclose(fp);
    return ;
}
void test_write_1(){
    Allocator* allocator = new Allocator("test");
    printf("init page_name:%s entry_path:%s \n",allocator->page_name,allocator->page_entry_path); 

    /*test write data*/
    for(int i=0;i<8;i++){
        char* ptr = (char*)allocator->psmalloc(13);
        if(!ptr){
            printf("psmalloc failed\n");
            exit(EXIT_FAILURE);
        }
        ptr[0] = '0'+i;
    }
    // throw -1;
    // exit(EXIT_SUCCESS);
    delete allocator;
}

void test_free_2(){
    Allocator* allocator = new Allocator("test");
    printf("init page_name:%s entry_path:%s \n",allocator->page_name,allocator->page_entry_path); 
    for(int i=0;i<8;i++){
        char* ptr = (char*)allocator->psmalloc(13);
        if(!ptr){
            printf("psmalloc failed\n");
            exit(EXIT_FAILURE);
        }
        ptr[0] = '0'+i;
        if(i%2)
        allocator->psfree(ptr);
    }
    
}
void test_free_1(){
    Allocator* allocator = new Allocator("test");
    printf("init page_name:%s entry_path:%s \n",allocator->page_name,allocator->page_entry_path); 

    allocator->psfree(1,0x40);
}
void test_psmalloc_1(){
    Allocator* allocator = new Allocator("test");
    printf("init page_name:%s entry_path:%s \n",allocator->page_name,allocator->page_entry_path); 

    /*test write data*/
    for(int i=0;i<8;i++){
        char* ptr = (char*)allocator->psmalloc(12);
        if(!ptr){
            printf("psmalloc failed\n");
            exit(EXIT_FAILURE);
        }
        ptr[0] = '0'+i;
    }

    /*test page index*/
    printf("%d\n", allocator->cur_page_id);
    printf("%p %p\n", allocator->page_index[allocator->cur_page_id]->ph, allocator->page_list[0]->ph);

    /*test map and unmap page*/
    printf("%c\n", *(char*)(allocator->ptr(allocator->cur_page_id, 16)));
    allocator->unmap_page(allocator->page_index[allocator->cur_page_id]);
    allocator->map_page(allocator->page_index[allocator->cur_page_id]);
    printf("%c\n", *(char*)(allocator->ptr(allocator->cur_page_id, 16)));
    /*test extend page_index*/
    // printf("%d %p %p\n", *((intptr_t*)allocator->page_index-1), allocator->page_index, (allocator->page_index[1])); 
    // allocator->max_page_id*=2;
    // allocator->page_index = (page**)realloc(allocator->page_index,allocator->max_page_id*sizeof(page*));
    // printf("%d %p %p\n", *((intptr_t*)allocator->page_index-1), allocator->page_index, (allocator->page_index[1])); 
}
class A{
public:
    int a;
    A(){
        printf("Construct\n");
        a = 1;
        this->~A();
    }

    ~A(){
         printf("DConstruct\n");
    }
};
int main(){
    // A* a = new A();
    // printf("%d\n", a->a);
    // delete a;
    // write_to_page();
    // read_page();
    // page_header ph = {PAGE_MAGIC,CHUNK_SIZE[0],PAGE_SIZE,CHUNK_SIZE[0]};
    // printf("%d \n",sizeof(ph));
    // test_psmalloc_1();
    // test_write_1();

    test_free_2();
    // test_free_1();
    // test_write_1();

    // char* ptr = (char*)malloc(100000000001);
    // printf("%p\n",ptr);
    // ptr[100000000000] = 'a';
    // printf("%c %p\n", ptr[100000000000], &ptr[100000000000]);
    // Allocator* allocator = new Allocator("test");
    // page_header ph;
    // printf("%d\n", sizeof(ph));
}