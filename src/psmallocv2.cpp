#include <psmallocv2.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <sys/mman.h> 
#include <unistd.h>
// #include <stdlib.h>
static inline size_t get_file_size(const char* file_path)
{
    struct stat st;
    memset(&st, 0, sizeof(st));
    stat(file_path, &st);
    return st.st_size;
}
int get_chunk_size_index(int chunk_size) {
  if (chunk_size == 16)
    return 0;
  else if (chunk_size == 32)
    return 1;
  else if (chunk_size == 64)
    return 2;
  else if (chunk_size == 128)
    return 3;
  else if (chunk_size == 256)
    return 4;
  else if (chunk_size == 512)
    return 5;
  else if (chunk_size == 1024)
    return 6;
  return -1;
}

page_head* Allocator::init_page(const char* filepath, int chunk_size)
{
    
    int fd = open (filepath, O_RDWR+O_CREAT,S_IREAD+S_IWRITE);
    if (fd == -1) {
        printf("can not open the file %s\n",filepath);
        return nullptr;
    }
    size_t page_size = get_file_size(filepath);
    if(page_size < PAGE_SIZE){
        page_size = PAGE_SIZE; 
        ftruncate(fd,page_size);
    }
    printf("[DEBUG] init page, filepath: %s, page size: %d\n", filepath, page_size);
    // printf("inti page, size: %d\n",page_size);

    page_head* ph = (page_head*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    // printf("%p\n",ptr);
    if(ph == MAP_FAILED){
        close(fd);
        return nullptr;
    }
    close(fd);

    ph->magic = PAGE_MAGIC;
    ph->chunk_size = chunk_size;
    ph->free_chunk_offset = chunk_size;
    ph->page_size = PAGE_SIZE;

    if(cur_page_id >= max_page_id){
        extend_page_index();
    }
    ph->page_id = ++cur_page_id;
    // struct pageHeader ph = {pageMagic,chunkSize,16};
    // printf("magic %d\n",ptr->magic);

    intptr_t chunk_current = chunk_size;
    intptr_t chunk_next = chunk_current + chunk_size;
    int chunk_num = PAGE_SIZE / chunk_size;
    for (int chunk_index = 0; chunk_index<chunk_num - 2; chunk_index++) {
        *(intptr_t*)(chunk_current+(intptr_t)ph) = chunk_next;
        chunk_current += chunk_size;
        chunk_next += chunk_size;
    }                        

    // set offset zero to the laskchunk
    *(intptr_t*)(chunk_current+(intptr_t)ph) = 0;
    
    return ph;
}

static int _map_page(page* pg, const char* filepath)
{
    int fd = open (filepath, O_RDWR);
    if (fd == -1) {
        printf("can not open the file %s\n",filepath);
        return -1;
    }
    
    size_t page_size = get_file_size(filepath);
    page_head* ph = (page_head*)mmap(NULL, page_size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    if(ph == MAP_FAILED){
        perror("mmap");
        close(fd);
        return -1;
    }
    close(fd);

    if(ph->magic!=PAGE_MAGIC){
        printf("ERROR: PAGE_MAGIC, %s\n",filepath);
        munmap(ph,page_size);
        return -1;
    }

    if(page_size != ph->page_size)
    {
        printf("ERROR: page size not equal, %s\n",filepath);
        munmap(ph,page_size);
        return -1;
    }
    pg->ph = ph;
    pg->is_mapped = true;
    return 0;
}
int Allocator::map_page(page* pg)
{
    return _map_page(pg, pg->path);
}

void Allocator::unmap_page(page* pg)
{
    munmap(pg->ph,pg->ph->page_size);
    pg->is_mapped = false;
}

static page_head* resize_page(page* pg, int new_size=0){
    // const char* filepath;
    int old_size = pg->ph->page_size;
    if(pg->is_mapped){
        munmap(pg->ph,old_size);
    }
    int fd = open (pg->path, O_RDWR);
    if (fd == -1) {
        printf("can not open the file %s\n",pg->path);
        return nullptr;
    }

    if(new_size==0) new_size = old_size * 2;

    printf("[Info] resize the page, old_size %d, new_size %d\n", old_size, new_size);
    ftruncate(fd,new_size);
    
    page_head* ph = (page_head*)mmap(NULL, new_size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    if(ph == MAP_FAILED){
        //TODO: 内存不足时，page回收策略
        perror("MAP_FAILED: ");
        close(fd);
        return nullptr;
    }
    close(fd);

    ph->page_size =new_size;
    ph->free_chunk_offset = old_size;
    int chunk_size = ph->chunk_size;

    intptr_t chunk_current = old_size;
    intptr_t chunk_next = chunk_current + chunk_size;
    for (int chunk_index = 0; chunk_index < ((new_size-old_size) / chunk_size) - 1; chunk_index++) {
        *(intptr_t*)(chunk_current+(intptr_t)ph) = chunk_next;
        chunk_current += chunk_size;
        chunk_next += chunk_size;
    }                                                                                            

    *(intptr_t*)(chunk_current+(intptr_t)ph) = 0;
    return ph;
}
static void write_line_to_file(const char* path, const char* line){
    FILE* fp;
    fp = fopen(path, "a+");
    if(fp == NULL) {
        perror("can not open the file\n.");
        return;
        // return C_PAGE_INIT_ERR;
    }
    fprintf(fp,"%s\n",line);
    fclose(fp);
}
int Allocator::init(){

    FILE* fp;

    fp = fopen(this->page_entry_path, "a+");
    if(fp == NULL) {
        printf("can not open the file %s\n",page_entry_path);
        return C_PAGE_INIT_ERR;
    }

    char current_path[MAX_PATH] = {0};
    while (fgets(current_path, sizeof(current_path), fp) != NULL)
    {
        current_path[strlen(current_path)-1] = '\0';
        printf("[Info] load page:%s\n", current_path);
    	page* pg = (page*)std::malloc(sizeof(page));
        strcpy(pg->path,current_path);

        if(_map_page(pg, pg->path)==-1){
            return C_PAGE_INIT_ERR;
        }

        /*set page index*/
        cur_page_id = MAX(cur_page_id, pg->ph->page_id);
        if(cur_page_id >= max_page_id){
            extend_page_index();
        }
        if(page_index[pg->ph->page_id]){
            printf("[ERROR] page index conflict, page id:%d\n", pg->ph->page_id);
            return C_PAGE_INIT_ERR;
        }
        page_index[pg->ph->page_id] = pg;
        

        /*link chunks*/
        int idx = get_chunk_size_index(pg->ph->chunk_size);
        pg->next = page_list[idx];
        page_list[idx] = pg;
    }
    fclose(fp);
    return C_PAGE_INIT_OK;
}

void Allocator::clear(){
    printf("clear\n");
    for(int i=0;i<CHUNK_SIZE_NUM;i++){
        page* pg = page_list[i];
        while(pg){
            unmap_page(pg);
            // munmap(pg->ph, pg->ph->page_size);
            page* pg_next = pg->next;
            std::free(pg);
            pg = pg_next;
        }
    }
    // std::free(page_list);
    std::free(page_index);
}

static int get_chunk_size(int size) {
  int chunk_size = MIN_MALLOC_SIZE;
  while (chunk_size <= MAX_MALLOC_SIZE) {
    if (size <= chunk_size) return chunk_size;
    chunk_size *= 2;
  }
  return size;
}

#define PAGE_HEAD pg->ph
void* Allocator::psmalloc(size_t size){
    int chunk_size = get_chunk_size(size);
    if(size > MAX_MALLOC_SIZE){
        printf("MALLOC size > MAX_MALLOC_SIZE:%d",MAX_MALLOC_SIZE);
        return nullptr;
    }
    int idx = get_chunk_size_index(chunk_size);

    page* pg = page_list[idx];

    int i = 0;

    while(pg){
        i++;
        printf("search free chunk from page list, link len:%d\n", i);
        /*resize the page if there are no free chunks */
        if(PAGE_HEAD->free_chunk_offset==0 && PAGE_HEAD->page_size < MAX_PAGE_SIZE){

            // munmap(PAGE_HEAD,PAGE_HEAD->page_size);
            PAGE_HEAD = resize_page(pg);
        }

        /*use the free chunk*/
        if(PAGE_HEAD->free_chunk_offset!=0){
            if(!pg->is_mapped) {
                if(_map_page(pg, pg->path)==-1){
                    return nullptr;
                }
            }

            intptr_t ptr = (intptr_t)PAGE_HEAD + PAGE_HEAD->free_chunk_offset;
            PAGE_HEAD->free_chunk_offset = *(intptr_t*)ptr;
            return (void*)ptr;
        }

        if(pg->next) {
            pg = pg->next;
        }else{
            break;
        }
    }
    
    /*generate new page's pathname by page_name_{chunk_size}.pg.{idx}*/
    char new_page_path[MAX_PATH] = {0};
    sprintf(new_page_path,"%s_%d.pg.%d",page_name,chunk_size,cur_page_id+1);
    page_head* new_page_head = init_page(new_page_path, chunk_size);
    if(!new_page_head){
        printf("ERROR: add new page failed, %s\n",new_page_path);
        return nullptr;
    }
    write_line_to_file(page_entry_path, new_page_path);
    /*add new page to the page_list*/
    page* new_pg = (page*)std::malloc(sizeof(page));

    new_pg->ph = new_page_head;
    new_pg->next = page_list[idx];
    strcpy(new_pg->path,new_page_path);
    new_pg->is_mapped = true;

    page_list[idx] = new_pg;

    /*add page index*/
    page_index[new_pg->ph->page_id] = new_pg;

    /*update the free chunk offset*/
    intptr_t ptr = (intptr_t)new_page_head + new_page_head->free_chunk_offset;
    new_page_head->free_chunk_offset = *(intptr_t*)ptr;
    return (void*)ptr;
}

// static inline page_head* find_page_head(void* ptr) {
//     int32_t page_size = MAX_PAGE_SIZE;
//     while(page_size > MIN_PAGE_SIZE){
//         int32_t offset = (intptr_t)ptr % page_size;
//         page_head* ph = (page_head*)((char*)ptr - offset);
//         printf("[DEBUG] find_page_head, page size:%d, ptr:%p, page_head:%p\n", page_size, ptr, ph);
//         if(ph->magic==PAGE_MAGIC){
//             return ph;
//         }
//         page_size >>= 1;
//     }
// }
void Allocator::psfree(void* ptr) {
    if(!ptr) return;
    int32_t page_size = MAX_PAGE_SIZE;
    int32_t offset;
    page_head* ph;
    while(page_size > MIN_PAGE_SIZE){
        offset = (intptr_t)ptr % page_size;
        ph = (page_head*)((char*)ptr - offset);
        printf("[DEBUG] find_page_head, page size:%d, ptr:%p, page_head:%p\n", page_size, ptr, ph);
        if(ph->magic==PAGE_MAGIC){
            break;
        }
        page_size >>= 1;
    }

    *(intptr_t*)ptr = ph->free_chunk_offset;
    ph->free_chunk_offset = offset;
    // if(!is_mapped) {
    //     printf("ERROR: please init the allocator first\n");
    //     return;
    // }
    // *(intptr_t*)ptr = ph->free_chunk_offset;
    // ph->free_chunk_offset = (intptr_t)ptr-(intptr_t)ph;
    // printf("ptr:%p ph:%p offset:%d\n",ptr,ph,ph->free_chunk_offset);
}

void  Allocator::psfree(int32_t page_id, int32_t offset) {

    if(!page_index[page_id]->is_mapped) {
        printf("[ERROR]: page %d is not mapped\n", page_id);
        return;
    }

    /*update free chunk*/
    intptr_t* p = (intptr_t*)ptr(page_id, offset);
    *p = page_index[page_id]->ph->free_chunk_offset;
    page_index[page_id]->ph->free_chunk_offset = offset;

    printf("[DEBUG] psfree, page id:%d offset:%d\n",page_id, offset);
    /*TODO: 将释放内存的当前页移动到free list的链头*/
}
void* Allocator::ptr(int32_t page_id, int32_t offset) {
    if(page_id > cur_page_id || !page_index[page_id]) return nullptr;
    if(!page_index[page_id]->is_mapped){
        printf("[INFO] unmap, path:%s, page_id:%d \n", page_index[page_id]->path, page_id);
        return nullptr;
    }
    return (char*)page_index[page_id]->ph + offset;
}


inline void Allocator::extend_page_index() {
    printf("[DEBUG] extend max page id from %d to %d\n", max_page_id, max_page_id*2);
    max_page_id*=2;
    page_index = (page**)realloc(page_index,max_page_id*sizeof(page*));
}