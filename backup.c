#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/blk_types.h>
#include <linux/err.h>

#include "csl.h"

/*
* read_from_file() - Get Data From filename
* @filename : target file name
* @data : pointer of data array
* @size : size of data
*/

int read_from_file(char* filename, void* data, size_t size)
{	
	struct file* file;
	loff_t pos = 0;

	file = filp_open(filename, O_RDONLY, 0644);

	if(IS_ERR(file)|| file == NULL){
		pr_warn(FILE_OPEN_ERROR_MSG);
		return FAIL_EXIT;
	}
		
	if(kernel_read(file, data, size, &pos) < 0){
		pr_warn(FILE_READ_ERROR_MSG);
		return FAIL_EXIT;
	}

	return SUCCESS_EXIT;
}


/*
* csl_restore() : Get Device Backup Data
* @dev : the struct of devcie to store data
*
* Find Backup File and restore it to device struct. 
* If there is not backup file, just initilaize xarray, linked list.
*/
void csl_restore(struct csl_dev *dev)
{
	int i;

	unsigned int xa_entry_num = 0;
	unsigned int gc_entry_num = 0;
	unsigned int *metadata_ptr;
	u8 *data_ptr;

	struct l2b_item* l2b_item;
	struct list_item* item;

	u8 *total_data;
	unsigned int total_data_size;


	// 1. Read offset, the number of each xarray and list entry
	
	void* header_data = kmalloc(BACKUP_HEADER_SIZE, GFP_KERNEL);
	
	if(IS_ERR(header_data) || header_data == NULL){
		pr_warn();
		goto nofile;
	}

	if(read_from_file(BACKUP_FILE_PATH, header_data, BACKUP_HEADER_SIZE)<0){
		pr_warn(FILE_READ_ERROR_MSG);
		goto nofile;
	}

	memcpy(dev->free_map, header_data, FREE_MAP_SIZE);

	xa_entry_num = *(unsigned int*)(header_data + FREE_MAP_SIZE);
	gc_entry_num = *(unsigned int*)(header_data + FREE_MAP_SIZE + sizeof(xa_entry_num));
	
	total_data_size = BACKUP_HEADER_SIZE + (xa_entry_num * XA_ENTRY_SIZE) + (gc_entry_num * GC_ENTRY_SIZE) + DEVICE_TOTAL_SIZE;
	
	total_data = vmalloc(total_data_size);

	if(read_from_file(BACKUP_FILE_PATH, total_data, total_data_size) < 0){
		pr_warn(FILE_READ_ERROR_MSG);
		goto nofile;
	}
	
	// 2. Read XArray Data

	metadata_ptr = (unsigned int*)(total_data + BACKUP_HEADER_SIZE);
	
	xa_init(&dev->l2p_map);

	for(i = 0; i < xa_entry_num; i++){
		l2b_item = kmalloc(sizeof(struct l2b_item), GFP_KERNEL);
		if(IS_ERR(l2b_item) || l2b_item == NULL){
			pr_warn();
			goto nofile;
		}
		l2b_item->lba = (unsigned long)(*metadata_ptr++);
		l2b_item->ppn = *metadata_ptr++;

		xa_store(&dev->l2p_map, l2b_item->lba, (void*)l2b_item, GFP_KERNEL);
	}


	// 3. Read Linked List Data 
	
	INIT_LIST_HEAD(&dev->list);
	for(i = 0; i < gc_entry_num; i++){
		item = kmalloc(sizeof(struct list_item), GFP_KERNEL);
		if(IS_ERR(item) || item == NULL){
			pr_warn();
			goto nofile;
		}
		item->sector = *metadata_ptr++;
		list_add_tail(&item->list_head, &dev->list);
	}
	
	// 4. Read Actual Data

	data_ptr = (u8*)metadata_ptr;
	memcpy(dev->data, data_ptr, DEVICE_TOTAL_SIZE);
	
	vfree(total_data);
	display_index();
	pr_info("There are %d XArray Entry, %d GC Entry > total data size is [%d] bytes", xa_entry_num, gc_entry_num, total_data_size);
	pr_info("CSL : RESTORE COMPLETE");
	return;

nofile:
	pr_warn(BACKUP_FAIL_MSG);
	xa_init(&dev->l2p_map);
	INIT_LIST_HEAD(&dev->list);
	bitmap_zero(dev->free_map, DEV_SECTOR_NUM);
	return;
}

/*
* write_to_file() - Write Data
* @filename : target file name
* @data : pointer of data array
* @size : size of data
*/

int write_to_file(const char *filename, const void *data, size_t size) 
{
    struct file *file;
    loff_t pos = 0;

    file = filp_open(filename, O_WRONLY | O_CREAT, 0644);
    if (IS_ERR(file)) {
		pr_warn(FILE_OPEN_ERROR_MSG);
        return FAIL_EXIT;
    }

    if(kernel_write(file, data, size, &pos) < 0){
		pr_warn(FILE_WRITE_ERROR_MSG);
		return FAIL_EXIT;
	}

    return SUCCESS_EXIT;
}

/*
* csl_backup() : Make Device Backup File
*
* Make Backup file for CSL device. 
* It contains device offset (for page mapping), the entry number and value of XArray and linked list and actual data array.
*/
void csl_backup(struct csl_dev *dev)
{
	unsigned int xa_entry_num=0;
	unsigned int gc_entry_num=0;
	unsigned int *metadata_ptr;

	void *data_ptr;
	struct list_item *litem;
	struct l2b_item* xa_item;

	u8 *total_data;
	unsigned int total_data_size = 0;

	// 1. Get the number of XArray entry.
	void* xa_ret;
	unsigned long idx;
	if(!xa_empty(&dev->l2p_map)){
		xa_for_each(&dev->l2p_map, idx, xa_ret){
			xa_entry_num++;
		}
	}
	
	// 2. Get the number of Linked List entry.
	if(!list_empty(&dev->list)){
		gc_entry_num = list_count_nodes(&dev->list);
	}
	
	
	// 3. Make a array for store data and copy header data
	total_data_size = BACKUP_HEADER_SIZE + (xa_entry_num * XA_ENTRY_SIZE) + (gc_entry_num * GC_ENTRY_SIZE) + DEVICE_TOTAL_SIZE;
	total_data = vmalloc(total_data_size);
	
	if(IS_ERR(total_data) || total_data < 0 || total_data == NULL){
		pr_info(MALLOC_ERROR_MSG);
		return;
	}
	
	memcpy(total_data, dev->free_map, FREE_MAP_SIZE);
	memcpy(total_data + FREE_MAP_SIZE, &xa_entry_num, sizeof(xa_entry_num));
	memcpy(total_data + FREE_MAP_SIZE + sizeof(xa_entry_num), &gc_entry_num, sizeof(gc_entry_num));
	
	// 4. Copy XArray, Linked List value 
	metadata_ptr = (unsigned int *)(total_data + BACKUP_HEADER_SIZE);

	xa_for_each(&dev->l2p_map, idx, xa_ret){
		xa_item = (struct l2b_item*)xa_ret;
		*metadata_ptr++ = (unsigned int)xa_item->lba;;
		*metadata_ptr++ = xa_item->ppn;
	}
	
	metadata_ptr = (unsigned int*)(total_data + BACKUP_HEADER_SIZE + xa_entry_num * XA_ENTRY_SIZE);
	
	list_for_each_entry(litem, &dev->list, list_head){
		*metadata_ptr++ = litem->sector;
	}
	
	// 5. Copy Actual data array
	data_ptr = total_data + BACKUP_HEADER_SIZE + (xa_entry_num * XA_ENTRY_SIZE) + (gc_entry_num * GC_ENTRY_SIZE);
    memcpy(data_ptr, dev->data, DEVICE_TOTAL_SIZE);

	if (write_to_file(BACKUP_FILE_PATH, total_data, total_data_size) < 0) {
        pr_warn(FILE_WRITE_ERROR_MSG);
		return;
    }

	vfree(total_data);

	display_index();
	
	pr_info("CSL : BACKUP COMPLETE");
	pr_info("There are %d XArray Entry, %d GC Entry > total data size is [%d] bytes", xa_entry_num, gc_entry_num, total_data_size);
}
