#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/blk_types.h>
#include <linux/err.h>
#include <linux/bitmap.h>
#include <linux/spinlock.h>

#include "csl.h"

static int CSL_MAJOR = 0; // save the major number of the device

struct csl_dev *dev;
DEFINE_SPINLOCK(csl_lock);

struct queue_limits queue_limit = {
		.logical_block_size	= 512,
	};

/**
 * find_free_sector() : find free sectors matching with given size. 
 * 
 * @size : the number of sectors we need
 *  
 */
unsigned long find_free_sector(unsigned int size)
{
	/* check there is free sector */
	if(bitmap_full(dev->free_map, DEV_SECTOR_NUM)) return OUT_OF_SECTOR;

	unsigned long bit;

	bit = bitmap_find_next_zero_area(dev->free_map, DEV_SECTOR_NUM, 0, size, 0);
	
	if(bit > DEV_SECTOR_NUM - size) return FAIL_EXIT; // check it is valid sector start number

	bitmap_set(dev->free_map, bit, size); 

	return bit;
}

/*
* Display current L2P Map array
*/
void display_index(void)
{
    unsigned long lba;
    void* ret;
    struct l2b_item* data;

    if(xa_empty(&dev->l2p_map)){
        pr_info("CSL : THERE IS NO ALLOCATE SECTOR");
        return;
    }

    pr_info("CSL : MAPPING INFO");
    pr_info("-----------------------------------------");
    pr_info("-----------------------------------------");
    pr_info("     %-10s   |     %-10s  |", "LBA", "PPN");
    pr_info("-----------------------------------------");

    xa_lock(&dev->l2p_map);
    xa_for_each(&dev->l2p_map, lba, ret)
    {
        data = (struct l2b_item*) ret;
        pr_info("     %-10lu   |     %-10d   |", data->lba, data->ppn);
    }

    pr_info("-----------------------------------------");
    xa_unlock(&dev->l2p_map);
}


/*
* csl_gc() : Operate Garbage Collection to use a free page
* return : a sector number of free page 
*/

uint csl_gc(void)
{
	struct list_item *entry;

	if(!list_empty(&dev->list))
	{
		unsigned int ppn_new;
		entry = list_first_entry(&dev->list, struct list_item, list_head);
		ppn_new = entry->sector;
		list_del(&entry->list_head);
		return ppn_new;
	}

	return OUT_OF_SECTOR;
}

/** 
* csl_invalidate() : Invalidate a sector
* @ppn : the sector number
**/

void csl_invalidate(unsigned int ppn)
{
	struct list_item *item;

	item = kmalloc(sizeof(struct list_item), GFP_KERNEL);

	if(IS_ERR(item) || item == NULL) {
		pr_warn("CSL : Fail To Allocate list item !");
		return;
	}

	item->sector = ppn;
	list_add_tail(&item->list_head, &dev->list);
	
}

/**
 * csl_read() : Read to buf
 * 
 * @ppn : the start sector number
 * @buf : a pointer of buffer to save the data
 * @num_sec : how many sectors to read 
 */
void csl_read(uint ppn, void* buf, uint num_sec)
{
	uint nbytes = num_sec * SECTOR_SIZE;

	if (ppn >= DEV_SECTOR_NUM){
		printk(KERN_WARNING "Wrong Sector num!");
		return;	
	}

	memcpy(buf, dev->data+(ppn*SECTOR_SIZE), nbytes);
}

/**
 * csl_write() : Write from data
 * 
 * @ppn : the start sector number
 * @buf : a pointer of buffer which have the data
 * @num_sec : how many sectors to write
 */
unsigned int csl_write(void* buf, uint num_sec)
{
	uint ppn;
	uint nbytes = num_sec * SECTOR_SIZE;

	ppn = find_free_sector(num_sec);

	/* There is no free sector > Do garbage collection */
	if (ppn >= DEV_SECTOR_NUM ){
		uint ppn_new = csl_gc();
		if(ppn_new >= DEV_SECTOR_NUM || ppn_new*SECTOR_SIZE + nbytes > DEVICE_TOTAL_SIZE){
			pr_warn("THERE IS NO CAPACITY IN CSL!");
			return OUT_OF_SECTOR;
		}
		
		memcpy((void*)dev->data+(ppn_new*SECTOR_SIZE), buf, nbytes);
		return ppn_new; 
	}

	/* There is Free sector */
	if(ppn >= DEV_SECTOR_NUM || ppn*SECTOR_SIZE + nbytes > DEVICE_TOTAL_SIZE){
			pr_warn("THERE IS NO CAPACITY IN CSL!");
			return OUT_OF_SECTOR;
	}
	memcpy((void*)dev->data+(ppn*SECTOR_SIZE), buf, nbytes);

	return ppn;	
}

static int csl_open(struct gendisk *gdisk, fmode_t mode)
{
	if(!blk_get_queue(gdisk->queue)){
		pr_warn("fail to get queue");
		return -1;
	}
	printk("CSL Device Drive open !\n");
	return 0;
}

static void csl_release(struct gendisk *gd)
{
	blk_put_queue(gd->queue);
	printk("CSL Device Drive released!\n");
}

static int csl_ioctl(struct block_device *bdev, blk_mode_t mode, unsigned cmd, unsigned long arg)
{
	printk("CSL Device Drive ioctl!\n");
	return 0;
}

/**
 * csl_transfer() : check mapping information
 * 
 * @dev : struct of our device
 * @start_sec : the start sector number
 * @num_sec : how many sectors we have to read or write
 * @buffer : pointer of memory area we access
 * @isWrite : the request is read or write
 * 
 */
void csl_transfer(unsigned int start_sec, unsigned int num_sec, void* buffer, int isWrite){

	struct l2b_item* l2b_item;
	void* ret;
	uint final_ppn;

	if(isWrite){
		ret = xa_load(&dev->l2p_map, (unsigned long)start_sec);
		
		/* There is no existing mapping information */
		if(!ret){
			final_ppn = csl_write(buffer, num_sec);
			if(final_ppn > DEV_SECTOR_NUM) return;

			/* Success to write > add new mapping information */	
			l2b_item = kmalloc(sizeof(struct l2b_item), GFP_KERNEL);

			if(IS_ERR(l2b_item) || l2b_item == NULL){
				pr_warn(MALLOC_ERROR_MSG);
				return;
			}

			l2b_item->lba = start_sec;
			l2b_item->ppn = final_ppn;

			xa_store(&dev->l2p_map, l2b_item->lba, (void*)l2b_item, GFP_KERNEL);
		}

		/* There is existing mapping information */
		else{
			l2b_item = (struct l2b_item*) ret;
			
			unsigned int ppn_old = l2b_item->ppn;
			final_ppn = csl_write(buffer, num_sec);
			if(final_ppn > DEV_SECTOR_NUM) return;
			
			/* Write Success > Invalidate existing ppn and update mapping information */
			l2b_item->ppn = final_ppn;
			csl_invalidate(ppn_old);
		}
		//pr_info("CSL : write start[%d] | num_sec[%d] | ppn[%d]", start_sec, num_sec, final_ppn);
	}

	else {
		ret = xa_load(&dev->l2p_map, start_sec);
		if(IS_ERR(ret) || ret == NULL) return; // There is no mapping information 
		l2b_item = (struct l2b_item*) ret;
		csl_read(l2b_item->ppn, buffer, num_sec);
	}
	
}

/**
 * csl_get_request() : get request and split it into bio
 * 
 * @rq : request we have to split
 * 
 */
void csl_get_request(struct request *rq)
{
	
	int isWrite = rq_data_dir(rq);

	sector_t start_sector = blk_rq_pos(rq);
	
	struct bio_vec bvec;
	struct req_iterator iter;

	void* buffer;

	rq_for_each_segment(bvec, rq, iter){
		unsigned int num_sector = blk_rq_cur_sectors(rq); 

		buffer = page_address(bvec.bv_page)+bvec.bv_offset;

		csl_transfer(start_sector, num_sector, buffer, isWrite); // transfer로 들어가면 read or write를 실행

		start_sector += num_sector; 
	}
}

/**
 * csl_enqueue() : get the request from request queue
 */
blk_status_t csl_enqueue(struct blk_mq_hw_ctx *ctx, const struct blk_mq_queue_data *data){
	struct request *rq = data->rq;
	
	blk_mq_start_request(rq);
	
	spin_lock(&csl_lock);
	csl_get_request(rq);
	spin_unlock(&csl_lock);

	blk_mq_end_request(rq, BLK_STS_OK);

	return BLK_STS_OK;
}

static struct block_device_operations csl_fops = {
	.owner = THIS_MODULE,
	.open = csl_open,
	.release = csl_release,
	.ioctl = csl_ioctl
};


static struct blk_mq_ops csl_mq_ops = {
	.queue_rq = csl_enqueue
};

static struct csl_dev *csl_alloc(void)
{
	struct csl_dev *mydev;
	struct gendisk *disk;

	int error;

	/* Allocate device information space */
	mydev = kzalloc(sizeof(struct csl_dev), GFP_KERNEL);

	/* Allocate tag set*/
	mydev->tag_set.ops = &csl_mq_ops;
	mydev->tag_set.nr_hw_queues = 1;
	mydev->tag_set.queue_depth = 128;
	mydev->tag_set.numa_node = NUMA_NO_NODE;
	mydev->tag_set.cmd_size = 0;
	mydev->tag_set.flags = BLK_MQ_F_SHOULD_MERGE;
	mydev->tag_set.driver_data = mydev;

	error = blk_mq_alloc_tag_set(&mydev->tag_set);

	if(error){
		kfree(mydev);
		return NULL;
	}


	/* Allocate disk */
	
	disk = blk_mq_alloc_disk(&mydev->tag_set, &queue_limit, mydev->queue);
	
	disk->major = CSL_MAJOR;
	disk->fops = &csl_fops;
	disk->first_minor = DEV_FIRST_MINOR;
	disk->minors = DEV_MINORS;
	disk->private_data = mydev;

	snprintf(disk->disk_name, 32, "CSL");

	if(IS_ERR(disk)){
		blk_mq_free_tag_set(&mydev->tag_set);
		kfree(mydev);
		return NULL;
	}

	mydev->gdisk = disk;
	mydev->queue = disk->queue;

	error = add_disk(disk); 
	if(error){
		blk_mq_free_tag_set(&mydev->tag_set);
		kfree(mydev);
		return NULL;
	}
	set_capacity(mydev->gdisk, DEV_SECTOR_NUM);

	/* Allocate bitmap and Actual data space */
	mydev->data = vmalloc(DEVICE_TOTAL_SIZE);
	mydev->free_map = bitmap_alloc(DEV_SECTOR_NUM, GFP_KERNEL);

	/* init lock*/
	spin_lock_init(&mydev->csl_lock);

	return mydev;
}

static int __init csl_init(void)
{	
	
	pr_info("CSL : CSL INITIALIZE START");

	int result;
	
	struct csl_dev *mydev;

	result = register_blkdev(CSL_MAJOR, DEV_NAME);
	
	// error handling
	if(result < 0)	{
		printk(KERN_WARNING "CSL: Fail to get major number!\n");
		return result;
	}

	if(CSL_MAJOR == 0){
		CSL_MAJOR = result;
	}
	
	mydev = csl_alloc();

	if(!mydev){
		printk(KERN_WARNING "CSL: Fail to add disk!\n");
		return -1;
	}
	
	dev = mydev;

	/* Get Backup data */
	csl_restore(dev);
	
	printk(KERN_INFO "DEVICE : CSL is successfully initialized with major number %d, SECTOR NUM : %d, free_sector = %ld\n",CSL_MAJOR,DEV_SECTOR_NUM, FREE_MAP_SIZE);
	return 0;
}


static void csl_free(void)
{
	blk_mq_destroy_queue(dev->queue);
	xa_destroy(&dev->l2p_map);
	unregister_blkdev(CSL_MAJOR,DEV_NAME);
	blk_mq_free_tag_set(&dev->tag_set);

	struct list_head *e, *tmp;
	list_for_each_safe(e, tmp, &dev->list){
		list_del(e);
		kfree(list_entry(e, struct list_item, list_head));
	}
	return;
}
static void __exit csl_exit(void)
{
	csl_backup(dev);
	del_gendisk(dev->gdisk);
	put_disk(dev->gdisk);

	csl_free();

	printk(KERN_INFO "DEVICE : CSL is successfully unregistered!\n");
	kfree(dev);
}

module_init(csl_init);
module_exit(csl_exit);

MODULE_AUTHOR("MinyoungKim");
MODULE_DESCRIPTION("Virtual Block Device Driver");
MODULE_LICENSE("GPL");
