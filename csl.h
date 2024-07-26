
#include <linux/spinlock.h>
#include <linux/blk-mq.h>
#include <linux/xarray.h>
#include <linux/list.h>
#include <linux/bitmap.h>

MODULE_AUTHOR("MinyoungKim");
MODULE_DESCRIPTION("Virtual Block Device Driver");
MODULE_LICENSE("GPL");

#define DEV_NAME "CSL"
#define DEVICE_TOTAL_SIZE 16*1024*1024 // 16MB
#define QUEUE_LIMIT 128
#define DEV_FIRST_MINOR 0
#define DEV_MINORS 16

#define SIZE_OF_SECTOR 512
#define DEV_SECTOR_NUM DEVICE_TOTAL_SIZE/SIZE_OF_SECTOR
#define BACKUP_FILE_PATH "/dev/csl_backup"
#define FREE_MAP_SIZE BITS_TO_LONGS(DEV_SECTOR_NUM) * sizeof(unsigned long)


/**
 * RETURN VALUE
 */
#define SUCCESS_EXIT 0
#define FAIL_EXIT -1
#define OUT_OF_SECTOR DEV_SECTOR_NUM + 10

/*
* DEVICE BACKUP CONSTANT
*/
#define BACKUP_HEADER_SIZE 2 * sizeof(unsigned int) + FREE_MAP_SIZE
#define XA_ENTRY_SIZE 2 * sizeof(unsigned int)
#define GC_ENTRY_SIZE sizeof(unsigned int)


/*
* ERROR MSG
*/
#define FILE_OPEN_ERROR_MSG "CSL : FAIL TO OPEN FILE"
#define FILE_READ_ERROR_MSG "CSL : FAIL TO READ FILE"
#define FILE_WRITE_ERROR_MSG "CSL : FAIL TO WRITE FILE"

#define MALLOC_ERROR_MSG "CSL : FAIL TO MALLOC SPACE"

#define BACKUP_FAIL_MSG "CSL : FAIL TO BACK UP CSL"

struct csl_dev{
	struct request_queue *queue;
	struct gendisk *gdisk;
	
	struct blk_mq_tag_set tag_set; // request queue의 tag set

	spinlock_t csl_lock;

	// Bitmap for manage free sectors
	unsigned long *free_map; 
	
	// Doubly linked list for garbage collection 
	struct list_head list;

	// XArray for logical block to physical page
	struct xarray l2p_map;

	// Actual Data Array
	u8 *data;
};

struct l2b_item{
	unsigned long lba; 
	unsigned int ppn;
};

struct list_item{
	unsigned int sector;
	struct list_head list_head;
};


/**
 * The function of csl.c
 * Block operation of device
 */
unsigned long find_free_sector(unsigned int size);
void display_index(void);
uint csl_gc(void);
void csl_invalidate(unsigned int ppn);
void csl_read(uint ppn, void* buf, uint num_sec);
unsigned int csl_write(void* buf, uint num_sec);
void csl_transfer(unsigned int start_sec, unsigned int num_sec, void* buffer, int isWrite);
void csl_get_request(struct request *rq);
blk_status_t csl_enqueue(struct blk_mq_hw_ctx *ctx, const struct blk_mq_queue_data *data);
void bits_print(unsigned long *v, u32 nbits);


//The functions of backup.c

int read_from_file(char* filename, void* data, size_t size);
void csl_restore(struct csl_dev *dev);
int write_to_file(const char *filename, const void *data, size_t size);
void csl_backup(struct csl_dev *dev);

