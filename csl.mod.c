#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_MITIGATION_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x5b9ad872, "filp_open" },
	{ 0xa4135f6f, "device_add_disk" },
	{ 0x7b37d4a7, "_find_first_zero_bit" },
	{ 0x69acdf38, "memcpy" },
	{ 0x37a0cba, "kfree" },
	{ 0x7461945f, "blk_mq_end_request" },
	{ 0x1acec83b, "__blk_mq_alloc_disk" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x60cbfd3b, "blk_mq_alloc_tag_set" },
	{ 0x122c3a7e, "_printk" },
	{ 0xd7bdc72, "put_disk" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x20dbf27, "bitmap_alloc" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0x615911d7, "__bitmap_set" },
	{ 0xb5a459dc, "unregister_blkdev" },
	{ 0x8bbe7f54, "set_capacity" },
	{ 0xa85a3e6d, "xa_load" },
	{ 0x94535adb, "blk_mq_free_tag_set" },
	{ 0x1820815b, "del_gendisk" },
	{ 0x64127b67, "bitmap_find_next_zero_area_off" },
	{ 0x7a3e22fc, "kernel_read" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0x999e8297, "vfree" },
	{ 0x720a27a7, "__register_blkdev" },
	{ 0xa36c9612, "blk_mq_destroy_queue" },
	{ 0xd9491c14, "xa_destroy" },
	{ 0x8fa25c24, "xa_find" },
	{ 0x5b3e282f, "xa_store" },
	{ 0x83f80e38, "blk_mq_start_request" },
	{ 0xdf36914b, "xa_find_after" },
	{ 0xa3e1b1c3, "kmalloc_trace" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0xb5b54b34, "_raw_spin_unlock" },
	{ 0x2e791fea, "kmalloc_caches" },
	{ 0xda0074b, "kernel_write" },
	{ 0x609ad8a4, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "BCA324B03C922C1E0A857BE");
