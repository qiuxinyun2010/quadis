#include <psmalloc.h>

int main()
{
    PageAllocator* page = new PageAllocator("test.p");

    //test init
    // page->init();
    // printf("%d %s\n",page->ph->chunk_size,page->filepath);

    //test1 malloc
    // int* ptr = (int*)page->malloc(40);
    // if(!ptr){
    //     printf("%s malloc failed\n",page->filepath);
    //     exit(EXIT_FAILURE);
    // }
    // printf("%p\n",ptr);
    // ptr[0] = 9;

    page->create(16,5);
    page->init();

    int* p1 = nullptr;
    int* ptr;
    for(int i=0;i<5;i++){
        ptr = (int*)page->malloc(sizeof(int));
        if(!ptr){
            printf("%s malloc failed\n",page->filepath);
            exit(EXIT_FAILURE);
        }
        ptr[0] = i+1;
        if(i==2) p1 = ptr;
    }
    printf("%p %p\n",ptr,p1);
    page->free(p1);
    // int* ptr = (int*)page->malloc(sizeof(int));
    // if(!ptr){
    //         printf("%s malloc failed\n",page->filepath);
    //         exit(EXIT_FAILURE);
    // }
    // ptr[0] = 6;
}