/*
 *  Copyright (C) 2008 dhewg, #wiidev efnet
 *  Copyright (C) 2008 marcan, #wiidev efnet
 *
 *  this file is part of the Homebrew Channel
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "system.h"
#include "stub_debug.h"
#include "usb.h"
#include "ios.h"
#include "cache.h"
#include "di.h"
#include "../config.h"

#define IOCTL_ES_LAUNCH					0x08
#define IOCTL_ES_GETVIEWCNT				0x12
#define IOCTL_ES_GETVIEWS				0x13

static struct ioctlv vecs[3] __attribute__((aligned(32)));

static u32 disc_id[0x40] __attribute__((aligned(32)));

static u8 tmd[0x5000] __attribute__((aligned(64)));

static struct {
	u32 count;
	u32 offset;
	u32 pad[6];
} part_table_info __attribute__((aligned(32)));

static struct {
	u32 offset;
	u32 type;
} partition_table[32] __attribute__((aligned(32)));

static struct
{
	char revision[16];
	void *entry;
	s32 size;
	s32 trailersize;
	s32 padding;
} apploader_hdr __attribute__((aligned(32)));

u64 *conf_magic = STUB_ADDR_MAGIC;
u64 *conf_titleID = STUB_ADDR_TITLE;

int es_fd;

int LaunchTitle(u64 titleID) {
	static u64 xtitleID __attribute__((aligned(32)));
	static u32 cntviews __attribute__((aligned(32)));
	static u8 tikviews[0xd8*4] __attribute__((aligned(32)));
	int ret;
	
	debug_string("LaunchTitle: ");
	debug_uint(titleID>>32);
	debug_string("-");
	debug_uint(titleID&0xFFFFFFFF);
	
	xtitleID = titleID;
	
	debug_string("\n\rGetTicketViewCount: ");
	
	vecs[0].data = &xtitleID;
	vecs[0].len = 8;
	vecs[1].data = &cntviews;
	vecs[1].len = 4;
	ret = ios_ioctlv(es_fd, IOCTL_ES_GETVIEWCNT, 1, 1, vecs);
	debug_uint(ret);
	debug_string(", views: ");
	debug_uint(cntviews);
	debug_string("\n\r");
	if(ret<0) return ret;
	if(cntviews>4) return -1;
		
	debug_string("GetTicketViews: ");
	vecs[0].data = &xtitleID;
	vecs[0].len = 8;
	vecs[1].data = &cntviews;
	vecs[1].len = 4;
	vecs[2].data = tikviews;
	vecs[2].len = 0xd8*cntviews;
	ret = ios_ioctlv(es_fd, IOCTL_ES_GETVIEWS, 2, 1, vecs);
	debug_uint(ret);
	debug_string("\n\r");
	if(ret<0) return ret;
	debug_string("Attempting to launch...\n\r");
	vecs[0].data = &xtitleID;
	vecs[0].len = 8;
	vecs[1].data = tikviews;
	vecs[1].len = 0xd8;
	ret = ios_ioctlvreboot(es_fd, IOCTL_ES_LAUNCH, 2, 0, vecs);
	if(ret < 0) {
		debug_string("Launch failed: ");
		debug_uint(ret);
		debug_string("\r\n");
	}
	return ret;
}

s32 IOS_GetVersion()
{
	u32 vercode;
	u16 version;
	DCInvalidateRange((void*)0x80003140,8);
	vercode = *((u32*)0x80003140);
	version = vercode >> 16;
	if(version == 0) return -1;
	if(version > 0xff) return -1;
	return version;
}

void printversion(void) {
	debug_string("IOS version: ");
	debug_uint(IOS_GetVersion());
	debug_string("\n\r");
}

int es_init(void) {
	debug_string("Opening /dev/es: ");
	es_fd = ios_open("/dev/es", 0);
	debug_uint(es_fd);
	debug_string("\n\r");
	return es_fd;
}

static void no_report(const char *fmt __attribute__((unused)), ...) { }

void _main (void) {
		int iosver;
		u64 titleID = MY_TITLEID;
		
        debug_string("\n\rHomebrew Channel stub code\n\r");
		
		if(*conf_magic == STUB_MAGIC) titleID = *conf_titleID;
			
		reset_ios();
		if(*(vu32*)0x8000180C == 1) //if Wii VC
		{
			//Code from TinyLoad - a simple region free (original) game launcher in 4k
			void (*app_entry)(void(**init)(void (*report)(const char *fmt, ...)), int (**main)(), void *(**final)());
			void (*app_init)(void (*report)(const char *fmt, ...));
			int (*app_main)(void **dst, int *size, int *offset);
			void *(*app_final)(void);
			void (*entrypoint)(void) __attribute__((noreturn));

			di_init();
			di_reset();
			di_identify();
			di_readdiscid(disc_id);
			di_unencryptedread(&part_table_info, sizeof(part_table_info), 0x10000);
			di_unencryptedread(partition_table, sizeof(partition_table), part_table_info.offset);
			int i;
			for(i=0; i<part_table_info.count; i++) {
				if(partition_table[i].type == 0) {
					break;
				}
			}
			di_openpartition(partition_table[i].offset, tmd);
			di_read((void*)0x80000000, 0x20, 0);
			di_read(&apploader_hdr, 0x20, 0x910);
			di_read((void*)0x81200000, apploader_hdr.size+apploader_hdr.trailersize, 0x918);
			ICInvalidateRange((void*)0x81200000, apploader_hdr.size+apploader_hdr.trailersize);

			app_entry = apploader_hdr.entry;
			app_entry(&app_init, &app_main, &app_final);
			app_init(no_report);

			while(1) {
				void *dst;
				int size;
				int offset;
				int res;

				res = app_main(&dst, &size, &offset);
				if(res != 1)
					break;
				di_read(dst, size, offset);
				ICInvalidateRange(dst, size);
			}
			di_shutdown();

			entrypoint = app_final();
			entrypoint();
		}
		else
		{
			if(es_init() < 0) goto fail;

			iosver = STUB_LOAD_IOS_VERSION;
			if(iosver < 0)
				iosver = 21; //bah
			printversion();
			debug_string("\n\rReloading IOS...\n\r");
			LaunchTitle(0x0000000100000000LL | iosver);
			printversion();

			if(es_init() < 0) goto fail;
			debug_string("\n\rLoading requested channel...\n\r");
			LaunchTitle(titleID);
			// if fail, try system menu
			debug_string("\n\rChannel load failed, trying with system menu...\n\r");
			LaunchTitle(0x0000000100000002LL);
			printversion();
		}
fail:
		debug_string("FAILURE\n\r");
		while(1);
}

