#include <psmalloc.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <sys/mman.h> 
#include <unistd.h>

bool PageAllocator::create(int chunk_size, int chunk_num=DEFAULT_CHUNK_NUM){
    int fd = open (filepath, O_RDWR+O_CREAT,S_IREAD+S_IWRITE);
    if (fd == -1) {
        printf("can not open the file %s\n",filepath);
        return false;
    }
    int page_size = chunk_size*chunk_num;
    ftruncate(fd,page_size);
    this->ph = (page_header*)mmap(NULL, page_size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    // printf("%p\n",ptr);
    if(this->ph == MAP_FAILED){
        perror("mmap");
        close(fd);
        return false;
    }
    close(fd);
    is_mapped = true;

    this->ph->magic = PAGE_MAGIC;
    this->ph->chunk_size = chunk_size;
    this->ph->free_chunk_offset = chunk_size;
    this->ph->page_size = page_size;
    // struct pageHeader ph = {pageMagic,chunkSize,16};
    // printf("magic %d\n",ptr->magic);

    intptr_t chunk_current = chunk_size;
    intptr_t chunk_next = chunk_current + chunk_size;

    for (int chunk_index = 0; chunk_index<chunk_num - 2; chunk_index++) {
        *(intptr_t*)(chunk_current+(intptr_t)this->ph) = chunk_next;
        chunk_current += chunk_size;
        chunk_next += chunk_size;
    }                                                                                            
    // // write the last chunk of page
    *(intptr_t*)(chunk_current+(intptr_t)this->ph) = 0;
    
    // munmap(ptr,pageSize);
    // pageFdSet[filename] = (intptr_t)ptr;
    return true;
}

static inline size_t get_file_size(const char* file_path)
{
    struct stat st;
    memset(&st, 0, sizeof(st));
    stat(file_path, &st);
    return st.st_size;
}

bool PageAllocator::init(){
    if(is_mapped) return true;

    int fd = open (filepath, O_RDWR);
    if (fd == -1) {
        printf("can not open the file %s\n",filepath);
        return false;
    }
    
    size_t page_size = get_file_size(filepath);
    ph = (page_header*)mmap(NULL, page_size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    if(ph == MAP_FAILED){
        perror("mmap");
        close(fd);
        return false;
    }
    close(fd);
    

    if(ph->magic!=PAGE_MAGIC){
        printf("ERROR: PAGE_MAGIC, %s\n",filepath);
        close_page();
        // munmap(ph,page_size);
        return false;
    }

    if(page_size != ph->page_size)
    {
        printf("ERROR: page size not equal, %s\n",filepath);
        munmap(ph,page_size);
        return false;
    }

    is_mapped = true;
    return true;
}

static int round_byte(int size) {
  int chunk_size = MIN_MALLOC_SIZE;
  while (chunk_size <= MAX_MALLOC_SIZE) {
    if (size <= chunk_size) return chunk_size;
    chunk_size *= 2;
  }
  return size;
}

void PageAllocator::close_page()
{
    munmap(ph,ph->page_size);
    is_mapped = false;
}

bool PageAllocator::extend()
{
        // ftruncate(fd,pageSize);
    if(!is_mapped){
        printf("ERROR: please init the allocator first\n");
        return false;
    }
    int page_size = ph->page_size;
    int chunk_size = ph->chunk_size;

    close_page();

    int fd = open (filepath, O_RDWR);
    if (fd == -1) {
        printf("can not open the file %s\n",filepath);
        return false;
    }
    ftruncate(fd,page_size*2);
    
    ph = (page_header*)mmap(NULL, page_size*2, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    if(ph == MAP_FAILED){
        perror("MAP_FAILED: ");
        return false;
    }
    close(fd);

    ph->page_size = page_size*2;
    ph->free_chunk_offset = page_size;
    intptr_t chunk_current = page_size;
    intptr_t chunk_next = chunk_current + chunk_size;
    for (int chunk_index = 0; chunk_index < (page_size / chunk_size) - 1; chunk_index++) {
        *(intptr_t*)(chunk_current+(intptr_t)ph) = chunk_next;
        chunk_current += chunk_size;
        chunk_next += chunk_size;
    }                                                                                            
    // // write the last chunk of page
    *(intptr_t*)(chunk_current+(intptr_t)ph) = 0;
    is_mapped = true;
    return true;
}

void* PageAllocator::malloc(size_t size)
{
    if(size > ph->chunk_size) {
        printf("ERROR: allocate size %d > the chunk_size % d\n",size,ph->chunk_size);
        return NULL;
    }
    if(!is_mapped){
        init();
        // printf("ERROR: please init the allocator first\n");
        // return NULL;
    }

    if(ph->free_chunk_offset==0)
    {
        printf("LOG: page memory full\n");
        if(!extend()){
            printf("ERROR: extend page failed\n");
            return NULL;
        }
    }

    intptr_t ptr = (intptr_t)ph + ph->free_chunk_offset;
    ph->free_chunk_offset = *(intptr_t*)ptr;
    return (void*)ptr;
}

void PageAllocator::free(void * ptr){
    if(!ptr) return;
    if(!is_mapped) {
        printf("ERROR: please init the allocator first\n");
        return;
    }
    *(intptr_t*)ptr = ph->free_chunk_offset;
    ph->free_chunk_offset = (intptr_t)ptr-(intptr_t)ph;
    printf("ptr:%p ph:%p offset:%d\n",ptr,ph,ph->free_chunk_offset);
    // int offset = ptr-(void *)ph; 
}