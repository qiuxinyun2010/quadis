#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>

void MmapFile(){
    int *p;
    int fd = open("hello",O_RDWR);
    if(fd <  0){
        perror("open hello");
        exit(1);
    }

    p = mmap(NULL,6,PROT_WRITE,MAP_SHARED,fd,0);
    if(p == MAP_FAILED){
        perror("mmap");
    }

    close(fd);
    p[0] = 0x30313233;
    munmap(p,6);

}


int main(int argc, char const *argv[])
{
    MmapFile();
    return 0;
}