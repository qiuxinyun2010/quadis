#include "util.h"
#include <sys/stat.h> 
#include "redisassert.h"
#include <sys/mman.h> 
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

size_t get_file_size(const char* file_path)
{
    struct stat st;
    memset(&st, 0, sizeof(st));
    stat(file_path, &st);
    return st.st_size;
}

void* create_and_map(const char* file_path, size_t page_size) {
    int fd = open (file_path, O_RDWR+O_CREAT, __S_IWRITE | __S_IREAD);
    if (fd == -1) {
        panic("can not open the file \n");
    }

    ftruncate(fd,page_size);
    printf("[DEBUG] create_and_map page, filepath: %s, page size: %ld\n", file_path, page_size);

    void* ptr = mmap(NULL, page_size, PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
    // printf("%p\n",ptr);
    if(ptr == MAP_FAILED){
        /*TODO: memory recycle*/
        panic("map page failed \n");
    }
    close(fd);
    return ptr;
}

/*文件存在：1 不存在：0*/
int check_file_existed(const char* file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        return 0;
    } else {
        fclose(file);
        return 1;
    }
}

ps_ptr ps_stringFromLongLong(long long value) {
    char buf[32];
    int len;
    char *s;

    len = sprintf(buf,"%lld",value);
    ps_ptr cursor = psmalloc(len+1);
    s = (char*)void_ptr(cursor);
    memcpy(s, buf, len);
    s[len] = '\0';
    return cursor;
}

char *stringFromLongLong(long long value) {
    char buf[32];
    int len;
    char *s;

    len = sprintf(buf,"%lld",value);
    s = (char*)malloc(len+1);
    memcpy(s, buf, len);
    s[len] = '\0';
    return s;
}