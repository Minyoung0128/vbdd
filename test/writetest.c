#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define BUF_LEN      4096 // 8 sector
#define DEV_NAME     "/dev/CSL"
#define DEV_SIZE     (16*1024*1024) // 16 MB
#define SECTOR_SIZE  512
#define MAX_SECTORS  (DEV_SIZE / SECTOR_SIZE)
#define START_SECTOR 1
#define END_SECTOR   DEV_SIZE/SECTOR_SIZE -1

int main() {
    static char buf[BUF_LEN];
    int fd;
    off_t off;

    printf("Start to test\n");

    if ((fd = open(DEV_NAME, O_RDWR)) < 0) {
        perror("open error");
        return 1;
    }

    srand(time(NULL));

    for (int i = 0; i < 10000; i++) {

        int rand_sector = START_SECTOR + rand() % (END_SECTOR - START_SECTOR);
        int rand_size = (rand() % (BUF_LEN / SECTOR_SIZE) + 1) * SECTOR_SIZE;

        off = (off_t) rand_sector * SECTOR_SIZE;

        if (lseek(fd, off, SEEK_SET) < 0) {
            perror("lseek error");
            close(fd);
            return 1;
        }

        snprintf(buf, rand_size, "write to %d [size : %d]", rand_sector, rand_size);
        // printf("write to %d : %s [size : %d]\n", rand_sector, buf, rand_size);

        if (write(fd, buf, rand_size) < 0) {
            perror("Write Error");
            close(fd);
            return 1;
        }
    }

    close(fd);
    return 0;
}
