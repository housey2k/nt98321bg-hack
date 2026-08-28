/**
     @file show_logo.c
     @brief :dec /mnt/mtd/logo file and transfer to lcd fmt/dim fb0 buf
     Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdal.h"
#include "vendor_common.h"
#define LOGO_FILE_NAME  "/mnt/mtd/logo_720x576.jpg"
#define JPG_W   720
#define JPG_H   576
#define ALIGN16_UP(x)       ALIGN_CEIL(x, 16)      /* for 16 bytes alignment */
HD_RESULT show_logo(unsigned int buf_paddr, unsigned int buf_size, int fb_pxlfmt, int width, int height)
{
	FILE *logo_fd = NULL;
	INT32 logo_file_size  = 0;
	INT32 fread_sz = 0;
	unsigned int yuv_logo_size  = 0;
	UINT64 pool = HD_COMMON_MEM_USER_BLK;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_COMMON_MEM_VB_BLK dec_in_blk = 0;
	HD_COMMON_MEM_VB_BLK dec_out_blk = 0;
	HD_COMMON_MEM_VB_BLK vpe_out_blk = 0;
	void *dec_in_blk_va = 0;
	UINT32 dec_in_blk_pa = 0;
	void *vpe_out_blk_va = 0;
	UINT32 vpe_out_blk_pa = 0;
	HD_RESULT ret = HD_OK;
	HD_PATH_ID dec_path_id = 0;
	HD_PATH_ID vpe_path_id = 0;
	HD_VIDEODEC_PATH_CONFIG dec_config;
	HD_VIDEODEC_BS bs_in_buffer = {0};
	HD_VIDEO_FRAME dec_out_buffer = {0};
	unsigned int frame_buf_size = 0;
	HD_VIDEOPROC_CROP vpe_crop;
	HD_VIDEOPROC_OUT vpe_out;
	HD_VIDEO_FRAME scale_in_buffer;
	HD_VIDEO_FRAME scale_out_buffer;	
	
	ret = hd_videodec_init();
	if (ret != HD_OK) {
		printf("hd_videodec_init fail\n");
		goto exit;
	}
	ret = hd_videoproc_init();
	if (ret != HD_OK) {
		printf("hd_videoproc_init fail\n");
		goto exit;
	}
	if ((logo_fd = fopen(LOGO_FILE_NAME, "rb")) == NULL) {
		printf("[ERROR] Open %s failed!!\n", LOGO_FILE_NAME);
		goto exit;
	}
	//prepare jpg data
	fseek(logo_fd, 0, SEEK_END);
	logo_file_size = ftell(logo_fd);
	if (logo_file_size < 0) {
		printf("logo_file_size(%d) failed!!\n", logo_file_size);
		goto exit;
	}
	fseek(logo_fd, 0, SEEK_SET);

	dec_in_blk = hd_common_mem_get_block(pool, logo_file_size, ddr_id);
	if (HD_COMMON_MEM_VB_INVALID_BLK == dec_in_blk) {
		printf("hd_common_mem_get_block fail\r\n");
		goto exit;
	}
	dec_in_blk_pa = hd_common_mem_blk2pa(dec_in_blk);
	if (dec_in_blk_pa == 0) {
		printf("hd_common_mem_blk2pa fail, in_blk = %#lx\r\n", dec_in_blk);
		hd_common_mem_release_block(dec_in_blk);
		goto exit;
	}
	dec_in_blk_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, dec_in_blk_pa, logo_file_size);
	if ((fread_sz = fread(dec_in_blk_va, 1, logo_file_size, logo_fd)) != logo_file_size) {
		printf("check fread size(%d %d) fail\n", fread_sz, logo_file_size);
		goto exit;
	}
	//
	//dec jpg
	ret =  hd_videodec_open(HD_VIDEODEC_0_IN_0, HD_VIDEODEC_0_OUT_0, &dec_path_id);
	if (ret != HD_OK) {
		printf("hd_videodec_open fail\n");
		goto exit;
	}
	/* Set parameters */
	dec_config.max_mem.dim.w = ALIGN_CEIL_16(JPG_W);
	dec_config.max_mem.dim.h = ALIGN_CEIL_16(JPG_H);
	dec_config.max_mem.frame_rate = 30;
	dec_config.max_mem.max_ref_num = 1;
	dec_config.max_mem.codec_type = HD_CODEC_TYPE_JPEG;
	ret = hd_videodec_set(dec_path_id, HD_VIDEODEC_PARAM_PATH_CONFIG, &dec_config);
	if (ret != HD_OK) {
		printf("hd_videodec_set fail\n");
		goto exit;
	}
	ret = vendor_common_mem_cache_sync(dec_in_blk_va, logo_file_size, VENDOR_COMMON_MEM_DMA_TO_DEVICE);
	if (ret != HD_OK) {
		printf("hd_common_mem_cache_sync fail\n");
		goto exit;
	}
	bs_in_buffer.ddr_id = ddr_id;
	bs_in_buffer.size = logo_file_size;
	bs_in_buffer.phy_addr = dec_in_blk_pa;
	dec_out_buffer.sign   = MAKEFOURCC('J', 'P', 'G', 'D');
	dec_out_buffer.dim.w  = ALIGN_CEIL_16(JPG_W);
	dec_out_buffer.dim.h  = ALIGN_CEIL_16(JPG_H);
	dec_out_buffer.ddr_id = ddr_id;
	frame_buf_size = hd_common_mem_calc_buf_size(&dec_out_buffer);

	dec_out_blk = hd_common_mem_get_block(pool, frame_buf_size, ddr_id);
	if (HD_COMMON_MEM_VB_INVALID_BLK == dec_out_blk) {
		printf("hd_common_mem_get_block fail\n");
		goto exit;
	}
	dec_out_buffer.phy_addr[0] = hd_common_mem_blk2pa(dec_out_blk);
	if (dec_out_buffer.phy_addr[0] == 0) {
		printf("hd_common_mem_blk2pa fail, blk = %#lx\r\n", dec_out_blk);
		hd_common_mem_release_block(dec_out_blk);
		goto exit;
	}
	ret = hd_videodec_push_in_buf(dec_path_id, &bs_in_buffer, &dec_out_buffer, 1000);
	if (ret != HD_OK) {
		printf("hd_videodec_push_in_buf fail\n");
		goto exit;
	}

	/* Pull out buffer */
	ret = hd_videodec_pull_out_buf(dec_path_id, &dec_out_buffer, 1000);
	if (ret != HD_OK) {
		printf("hd_videodec_pull_out_buf fail\n");	
		goto exit;
	}
	//
	//vpe transfer fmt
	ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &vpe_path_id);
	if (ret != HD_OK) {
		printf("hd_videoproc_open fail\n");
		goto exit;
	}
	if (fb_pxlfmt == HD_VIDEO_PXLFMT_YUV420) {
		yuv_logo_size = (width * ALIGN_CEIL_16(height) * 3 / 2);
	} else if (fb_pxlfmt == HD_VIDEO_PXLFMT_YUV420_NVX3) {
		yuv_logo_size = ((width * ALIGN_CEIL_16(height) * 3 / 2) * 3 / 4);
	} else {
		yuv_logo_size = (width * ALIGN_CEIL_16(height) * 2);
	}
	vpe_out_blk = hd_common_mem_get_block(pool, yuv_logo_size, ddr_id);
	if (HD_COMMON_MEM_VB_INVALID_BLK == vpe_out_blk) {
		printf("hd_common_mem_get_block fail\n");
		goto exit;
	}
	vpe_out_blk_pa = hd_common_mem_blk2pa(vpe_out_blk);
	if (vpe_out_blk_pa == 0) {
		printf("hd_common_mem_blk2pa fail, blk = %#lx\r\n", vpe_out_blk);
		hd_common_mem_release_block(vpe_out_blk);
		goto exit;
	}
	vpe_out_blk_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, vpe_out_blk_pa, yuv_logo_size);
	
	vpe_crop.win.rect.x = 0;
	vpe_crop.win.rect.y = 0;
	vpe_crop.win.rect.w = dec_out_buffer.dim.w;
	vpe_crop.win.rect.h = dec_out_buffer.dim.h;
	vpe_crop.win.coord.w = dec_out_buffer.dim.w;
	vpe_crop.win.coord.h = dec_out_buffer.dim.h;
	ret = hd_videoproc_set(vpe_path_id, HD_VIDEOPROC_PARAM_IN_CROP, (void *)&vpe_crop);
	if (ret != HD_OK) {
		printf("hd_videoproc_set in crop fail\n");
		goto exit;
	}
	vpe_out.rect.x = 0;
	vpe_out.rect.y = 0;
	vpe_out.rect.w = width;
	vpe_out.rect.h = height;
	vpe_out.bg.w = width;
	vpe_out.bg.h = height;
	vpe_out.pxlfmt = fb_pxlfmt;
	vpe_out.dir = HD_VIDEO_DIR_NONE;

	ret = hd_videoproc_set(vpe_path_id, HD_VIDEOPROC_PARAM_OUT, (void *)&vpe_out);
	if (ret != HD_OK) {
		printf("hd_videoproc_set out fail\n");
	}
	memset(&scale_in_buffer, 0x0, sizeof(HD_VIDEO_FRAME));
	memset(&scale_out_buffer, 0x0, sizeof(HD_VIDEO_FRAME));
	
	scale_in_buffer.ddr_id = ddr_id;
	scale_in_buffer.dim.w = dec_out_buffer.dim.w;
	scale_in_buffer.dim.h = dec_out_buffer.dim.h;
	scale_in_buffer.pxlfmt = HD_VIDEO_PXLFMT_YUV420_W8;
	scale_in_buffer.phy_addr[0] = dec_out_buffer.phy_addr[0];
	scale_out_buffer.phy_addr[0] = vpe_out_blk_pa;

	ret = hd_videoproc_push_in_buf(vpe_path_id, &scale_in_buffer, &scale_out_buffer, 500);
	if (ret != HD_OK) {
		printf("hd_videoproc_push_in_buf fail\n");
		goto exit;
	}	
	ret = hd_videoproc_pull_out_buf(vpe_path_id, &scale_out_buffer, 500);
	if (ret != HD_OK) {
		printf("hd_videoproc_pull_out_buf fail\n");
		goto exit;
	}
	ret = hd_common_dmacopy(ddr_id, vpe_out_blk_pa, ddr_id, buf_paddr, yuv_logo_size);
	if (ret != HD_OK) {
		printf("hd_common_dmacopy fail\n");
	}

exit:
	if (logo_fd) {
		fclose(logo_fd);
	}
	ret = hd_common_mem_munmap(dec_in_blk_va, logo_file_size);
	if (ret != HD_OK) {
		printf("hd_common_mem_munmap fail\n");
	}
	ret = hd_common_mem_munmap(vpe_out_blk_va, yuv_logo_size);
	if (ret != HD_OK) {
		printf("hd_common_mem_munmap fail\n");
	}
	if (dec_in_blk) {
		ret = hd_common_mem_release_block(dec_in_blk);
		if (ret != HD_OK) {
			printf("hd_common_mem_release_block fail\n");
		}
	}
	if (dec_out_blk) {
		ret = hd_common_mem_release_block(dec_out_blk);
		if (ret != HD_OK) {
			printf("hd_common_mem_release_block fail\n");
		}
	}
	if (vpe_out_blk) {
		ret = hd_common_mem_release_block(vpe_out_blk);
		if (ret != HD_OK) {
			printf("hd_common_mem_release_block fail\n");
		}
	}
	if (dec_path_id) {
		ret = hd_videodec_close(dec_path_id);
		if (ret != HD_OK) {
			printf("hd_videodec_close fail\n");
		}
	}
	if (vpe_path_id) {
		ret = hd_videoproc_close(vpe_path_id);
		if (ret != HD_OK) {
			printf("hd_videoproc_close fail\n");
		}
	}
	ret = hd_videodec_uninit();
	if (ret != HD_OK) {
		printf("hd_videodec_uninit fail\n");
	}
	
	ret = hd_videoproc_uninit();
	if (ret != HD_OK) {
		printf("hd_videoproc_uninit fail\n");
	}
	return ret;
}