#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <sys/mman.h> 
#include <unistd.h>
#include <dirent.h>
#include "psmalloc.h"
#include "util.h"


#define MAX(a,b)            (((a) > (b)) ? (a) : (b))

// Round a value x up to the next multiple of y
#define ROUND_UP(x, y) ((x) % (y) == 0 ? (x) : (x) + ((y) - (x) % (y)))

#define pm page_manager::pm

page_manager* pm = nullptr;

/*private function declare*/
static int get_chunk_size(int size);
static int get_chunk_type(int size);
static page_head* _create_page(const char* filepath, int chunk_size, int page_id, int page_size);
static page_head* create_page(const char* filepath, int chunk_size, int page_id);
static page_head* resize_page(page* pg);
static page_head* _resize_page(page* pg, int new_size);
static int map_page(page* pg, const char* filepath);
static int _map_page(page* pg, const char* filepath, int fd);
static inline void extend_page_index(); 
static void write_line_to_file(const char* path, const char* line);

/* page_ptr */
inline void* void_ptr(ps_ptr ptr)  {

    assert(pm!=nullptr);
    assert(ptr!=0);

    if(!pm->page_index[ptr>>PAGE_OFFSET_BITS]){
        printf("[ERROR] pageid:%lu, offset:%lu\n", PAGE_ID(ptr), PAGE_OFFSET(ptr));
        panic("Page is not existed\n");
    }

    page* pg = pm->page_index[ptr>>PAGE_OFFSET_BITS];
    if(!pg->is_mapped) {
        printf("[DEBUG] page:%u is not mapped\n", pg->ph->page_id);
        if(map_page(pg, pg->path)==-1){
            panic("Map page failed\n");
        }
    }
    return (char*)pg->ph + PAGE_OFFSET(ptr);

}

/*API*/
#define PAGE_HEAD pg->ph

ps_ptr pscalloc(size_t size) {
    return psmalloc(size, PAGE_TYPE_CHUNK, 1);
}
ps_ptr psmalloc(size_t size){
    return psmalloc(size, PAGE_TYPE_CHUNK, 0);
}
ps_ptr psmalloc(size_t size, int page_type, int set_zero) {
    
    assert(pm!=nullptr);
    
    if(page_type == PAGE_TYPE_MAP || size > MAX_CHUNK_SIZE) {
        size = ROUND_UP(size,MIN_PAGE_SIZE);
        char new_page_path[MAX_PATH] = {0};
        extend_page_index();
        pm->cur_page_id++;
        sprintf(new_page_path,"%s/%lu.pg", DEFAULT_PAGE_DIR, pm->cur_page_id);
        page_head* new_page_head = _create_page(new_page_path, 0, pm->cur_page_id, size+sizeof(page_head));
        if(!new_page_head){
            printf("ERROR: add new page failed, %s\n",new_page_path);
            return 0;
        }

        page* new_pg = new page(new_page_head, new_page_path);
        new_pg->is_mapped = true;
        pm->page_index[new_pg->ph->page_id] = new_pg;

        return PAGE_PTR(pm->cur_page_id, sizeof(page_head));
    }

    int chunk_size = get_chunk_size(size);
    int idx = get_chunk_type(chunk_size);


    /*create new page's pathname by page_name_{chunk_size}.pg.{idx}*/
    if(pm->free_list[idx]==0) {

        
        char new_page_path[MAX_PATH] = {0};
        
        extend_page_index();
        pm->cur_page_id++;
        sprintf(new_page_path,"%s/%lu.pg", DEFAULT_PAGE_DIR, pm->cur_page_id);

        page_head* new_page_head = create_page(new_page_path, chunk_size, pm->cur_page_id);
        if(!new_page_head){
            printf("ERROR: add new page failed, %s\n",new_page_path);
            return 0;
        }
        // write_line_to_file(pm->page_entry_path, new_page_path);

        /*add new page to the page index*/
        page* new_pg = new page(new_page_head, new_page_path);

        new_pg->is_mapped = true;
        pm->page_index[new_pg->ph->page_id] = new_pg;
        pm->free_list[idx] = PAGE_PTR(new_pg->ph->page_id, sizeof(page_head));
        #ifdef PRINT_DEBUG
            printf("[DEBUG]: create new page, %s\n",new_pg->path);
        #endif

    }



    /*map page */
    page* pg = pm->page_index[PAGE_ID(pm->free_list[idx])];

    if(!pg->is_mapped) {
        printf("[DEBUG] page: is not mapped:%u\n", PAGE_HEAD->page_id);
        if(map_page(pg, pg->path)==-1){
            return 0;
        }
    }
    ps_ptr p = pm->free_list[idx];
    void* vp = void_ptr(p);
    pm->free_list[idx] = *(ps_ptr*)vp;

    if(set_zero) {
        memset(vp,0,size);
    }
    return p;
}

void psfree(ps_ptr ptr) {

    assert(pm!=nullptr);

    page* pg = pm->page_index[PAGE_ID(ptr)];

    if(!pg){
        printf("[ERROR] page_id:%lu\n",PAGE_ID(ptr));
        assert(pg!=nullptr);
    }
    

    if(PAGE_HEAD->page_type==PAGE_TYPE_MAP){
        munmap(PAGE_HEAD, PAGE_HEAD->page_size);
        remove(pg->path);
        free(pg);
        return ;
    }

   if(1) {
        memset(void_ptr(ptr),0,PAGE_HEAD->chunk_size);
    }

    int idx = get_chunk_type(PAGE_HEAD->chunk_size);
    *(ps_ptr*)void_ptr(ptr) = pm->free_list[idx];
    pm->free_list[idx] = ptr;
}
/*page_manager function*/
int page_manager::init(const char* page_entry_name) {
    if(pm) return -1;
    
    pm = new page_manager();

    #ifdef PRINT_DEBUG
    printf("[DEBUG] init page entry path: %s\n", page_entry_name);
    #endif

    pm->cur_page_id = 0;
    pm->max_page_id = DEFAULT_MAX_PAGE;
    pm->page_index = (page**)calloc(DEFAULT_MAX_PAGE, sizeof(page*));

    /*create save page dir*/
    DIR* folder = opendir(DEFAULT_PAGE_DIR);
    if (!folder) {
        printf("create save page dir:%s \n", DEFAULT_PAGE_DIR);
        int status = mkdir(DEFAULT_PAGE_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status != 0) {
            panic("Error save page dir\n");
        }
        folder = opendir(DEFAULT_PAGE_DIR);
    }

    struct dirent* entry;
    char current_path[MAX_PATH];
    page* pg;
    while ((entry = readdir(folder)) != NULL) {
        const char* filename = entry->d_name;
        if (strcmp(filename + strlen(filename) - strlen(PAGE_SUFFIX), PAGE_SUFFIX) == 0) {
            snprintf(current_path,MAX_PATH, "%s/%s", DEFAULT_PAGE_DIR, filename);
            #ifdef PRINT_DEBUG
            printf("[DEBUG] load page: %s\n", current_path);
            #endif
            
            pg = new page(nullptr,current_path);
            if(map_page(pg, pg->path)==-1){
                return C_PAGE_INIT_ERR;
            }
            

            pm->cur_page_id = MAX(pm->cur_page_id, pg->ph->page_id);
            extend_page_index();
            if(pm->page_index[pg->ph->page_id]){
                printf("[ERROR] page index conflict, page id:%u\n", pg->ph->page_id);
                return C_PAGE_INIT_ERR;
            }
            pm->page_index[pg->ph->page_id] = pg;
        }
        
    }

    /*check entry file exist*/
    char file_path[200];
    sprintf(file_path,"%s/%s", DEFAULT_PAGE_DIR, page_entry_name);

    FILE *file = fopen(file_path, "r");
    page_head* ph; 
    if (!file) {
        printf("File does not exist\n");
        ph = _create_page(file_path,0,0,MIN_PAGE_SIZE);
    } else {
        fclose(file);
        page pg;
        map_page(&pg, file_path);
        ph = pg.ph;
    }
    pm->free_list = (ps_ptr*)(ph+1);
    #ifdef PRINT_DEBUG
    printf("[DEBUG] cur_page_id:%lu\n",pm->cur_page_id);
    #endif
    return C_PAGE_INIT_OK;
}


/*private function*/
static int get_chunk_size(int size) {
  int chunk_size = MIN_MALLOC_SIZE;
  while (chunk_size <= MAX_CHUNK_SIZE) {
    if (size <= chunk_size) return chunk_size;
    chunk_size *= 2;
  }
  return size;
}

static int get_chunk_type(int chunk_size) {

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

static void write_line_to_file(const char* path, const char* line){
    FILE* fp;
    fp = fopen(path, "a+");
    if(fp == NULL) {
        perror("can not open the file\n.");
        return;
    }
    fprintf(fp,"%s\n",line);
    fclose(fp);
}

static inline void extend_page_index() {
    if(pm->cur_page_id >= pm->max_page_id){
        // extend_page_index();
        #ifdef PRINT_DEBUG
        printf("[DEBUG] extend max page id from %lu to %lu\n", pm->max_page_id, pm->cur_page_id*2);
        #endif
        pm->max_page_id = pm->cur_page_id*2;
        pm->page_index = (page**)realloc(pm->page_index,pm->max_page_id * sizeof(page*));
    }
}
static page_head* create_page(const char* filepath, int chunk_size, int page_id){
    return _create_page(filepath, chunk_size, page_id, PAGE_SIZE);
}
static page_head* _create_page(const char* filepath, int chunk_size, int page_id, int page_size)
{
    int fd = open (filepath, O_RDWR+O_CREAT, __S_IWRITE | __S_IREAD);
    if (fd == -1) {
        panic("can not open the file \n");
    }

    ftruncate(fd,page_size);
    #ifdef PRINT_DEBUG
        printf("[DEBUG] init page, filepath: %s, page size: %d, chunk size:%d \n", filepath, page_size, chunk_size);
    #endif

    page_head* ph = (page_head*)mmap(NULL, page_size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    // printf("%p\n",ptr);
    if(ph == MAP_FAILED){
        /*TODO: memory recycle*/
        panic("map page failed \n");
    }
    close(fd);

    ph->magic = PAGE_MAGIC;

   
    // ph->next_free_ptr = {page_id, sizeof(page_head)};
    ph->page_size = page_size;
    ph->page_id = page_id;
    ph->chunk_size = chunk_size;
    if (chunk_size == 0) {
        ph->page_type = PAGE_TYPE_MAP;
        return ph;
    }
    ph->page_type = PAGE_TYPE_CHUNK;

    intptr_t chunk_current = sizeof(page_head);
    intptr_t chunk_next = chunk_current + chunk_size;
    int chunk_num = PAGE_SIZE / chunk_size;
    for (int chunk_index = 0; chunk_index<chunk_num - 2; chunk_index++) {
        *(intptr_t*)(chunk_current+(intptr_t)ph) = PAGE_PTR(page_id, chunk_next);
        chunk_current += chunk_size;
        chunk_next += chunk_size;
    }                   

    // set offset zero to the laskchunk
    *(intptr_t*)(chunk_current+(intptr_t)ph) = 0;
    
    return ph;
}

static int _map_page(page* pg, const char* filepath, size_t page_size, int fd)
{
    if(fd==-1) fd = open (filepath, O_RDWR);
    if (fd == -1) {
        panic("can not open the file \n");
    }
    
    if(page_size == 0)
    page_size = get_file_size(filepath);

    page_head* ph = (page_head*)mmap(NULL, page_size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    if(ph == MAP_FAILED){
        /*TODO: memory recycle*/
        panic("map page failed \n");
    }
    close(fd);

    if(ph->magic!=PAGE_MAGIC){
        panic("page magic error\n");
    }

    if(page_size != ph->page_size)
    {
        panic("page size error\n");
    }
    pg->ph = ph;
    pg->is_mapped = 1;
    return 0;
}

static int map_page(page* pg, const char* filepath){
    return _map_page(pg, filepath, 0, -1);
}
static page_head* _resize_page(page* pg, int new_size){
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

static page_head* resize_page(page* pg) {
    return _resize_page(pg, 0);
}