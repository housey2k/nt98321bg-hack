/**
   @file dvr_perf.c

   @brief playback/liveview/encode perf sample.

   @author Ivan Wang

   @ingroup mhdal

   @note Nothing.

   Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <asm/ioctl.h>
#include "hdal.h"
#include "vendor_video.h"
#include "hd_debug.h"

#define DVR_PERF_VERSION	"v1.10"

#define CONFIG_ENCODE_H265
#define CONFIG_OSG

#define MAINSTREAM_WIDTH    1280
#define MAINSTREAM_HEIGHT   720
#define MAINSTREAM_LEN      ((MAINSTREAM_WIDTH * MAINSTREAM_HEIGHT * 3) / 2)

#define SUBSTREAM_WIDTH    	352
#define SUBSTREAM_HEIGHT    288
#define SUBSTREAM_LEN      ((SUBSTREAM_WIDTH * SUBSTREAM_HEIGHT * 3) / 2)
#define MAX_REC_BITSTREAM_NUM  4
#define MAX_PB_BITSTREAM_NUM   1
#define MAX_LV_BITSTREAM_NUM   4
#define ENC_FRAME_RATE         30

#define INTERVAL_TIME_MS    5000

#ifdef CONFIG_ENCODE_H264
	#define PATTERN_BS_FILE		"1920x1080_ref2.264"
	#define PATTERN_LEN_FILE	"1920x1080_ref2.len"
#else
	#define PATTERN_BS_FILE		"720P_30fps.h265"
	#define PATTERN_LEN_FILE	"720P_30fps.h265.len"
#endif

typedef struct _VIDEO_PLAYBACK {                            // video path
    HD_PATH_ID pb_dec_list[MAX_PB_BITSTREAM_NUM];
   	HD_PATH_ID pb_lcd_list[MAX_PB_BITSTREAM_NUM];
	HD_PATH_ID pb_vpe_list[MAX_PB_BITSTREAM_NUM];
	HD_PATH_ID video_out_ctrl;
	HD_VIDEOOUT_SYSCAPS lcd_syscaps;
	pthread_t playback_thread_id;
	int is_playback;
} VIDEO_PLAYBACK;

typedef struct _VIDEO_RECORD {
	HD_PATH_ID rec_cap_list[MAX_REC_BITSTREAM_NUM * 2];	//main+sub
	HD_PATH_ID rec_enc_list[MAX_REC_BITSTREAM_NUM * 2];	//main+sub
	HD_VIDEOCAP_SYSCAPS cap_syscaps[MAX_REC_BITSTREAM_NUM];
	pthread_t record_thread_id;
	int is_record;
	int enc_width[MAX_REC_BITSTREAM_NUM * 2];
	int enc_height[MAX_REC_BITSTREAM_NUM * 2];
	HD_PATH_ID rec_stamp_list[MAX_REC_BITSTREAM_NUM * 2];
} VIDEO_RECORD;

typedef struct _VIDEO_LIVEVIEW {
	HD_PATH_ID lv_cap_list[MAX_LV_BITSTREAM_NUM];
   	HD_PATH_ID lv_lcd_list[MAX_LV_BITSTREAM_NUM];
	HD_PATH_ID lv_vpe_list[MAX_LV_BITSTREAM_NUM];
	HD_PATH_ID video_out_ctrl;
	HD_VIDEOOUT_SYSCAPS lcd_syscaps;
	HD_VIDEOCAP_SYSCAPS cap_syscaps[MAX_LV_BITSTREAM_NUM];
	UINT32 tmnr_motion_ddr_id;
	UINT32 tmnr_motion_blk_size;
	UINT32 tmnr_motion_count;
	UINT32 tmnr_motion_start;
	int is_liveview;
} VIDEO_LIVEVIEW;

typedef struct {
	CHAR name[40];
	HD_DIM dim;
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 pa;
} STAMP_INFO;

static void *record_thread(void *arg_rec);
static void *playback_thread(void *arg_pb);

static UINT dvr_rec_bitstream_num = MAX_REC_BITSTREAM_NUM;
static UINT dvr_lv_bitstream_num = MAX_LV_BITSTREAM_NUM;
static UINT dvr_pb_bitstream_num = MAX_PB_BITSTREAM_NUM;

HD_RESULT get_lv_cap_caps(VIDEO_LIVEVIEW *lv)
{
	int i;
	HD_RESULT ret = HD_OK;
	
	for (i = 0; i < dvr_lv_bitstream_num; i++) {
		ret = hd_videocap_get(lv->lv_cap_list[i], HD_VIDEOCAP_PARAM_SYSCAPS, &lv->cap_syscaps[i]);
		if (ret != HD_OK) {
			printf("hd_videocap_get:param_id(%d) fail\n", HD_VIDEOCAP_PARAM_SYSCAPS);
			return ret;
		}
	}
	return ret;
}

HD_RESULT get_rec_cap_caps(VIDEO_RECORD *rec)
{
	int i;
	HD_RESULT ret = HD_OK;
	
	for (i = 0; i < dvr_lv_bitstream_num; i++) {
		ret = hd_videocap_get(rec->rec_cap_list[i], HD_VIDEOCAP_PARAM_SYSCAPS, &rec->cap_syscaps[i]);
		if (ret != HD_OK) {
			printf("hd_videocap_get:param_id(%d) fail\n", HD_VIDEOCAP_PARAM_SYSCAPS);
			return ret;
		}
		printf("videocap %d: %lu x %lu %lufps\n", i, rec->cap_syscaps[i].max_dim.w, rec->cap_syscaps[i].max_dim.h,
			rec->cap_syscaps[i].max_frame_rate);
	}
	return ret;
}

HD_RESULT get_lv_lcd_caps(VIDEO_LIVEVIEW *lv)
{
	HD_RESULT ret;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &lv->video_out_ctrl);
	if (ret != HD_OK) {
		printf("hd_videoout_get:param_id(%d) fail\n", HD_VIDEOOUT_PARAM_SYSCAPS);
		return ret;
	}
	ret = hd_videoout_get(lv->video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, &lv->lcd_syscaps);
	if (ret != HD_OK) {
		printf("hd_videoout_get:param_id(%d) fail\n", HD_VIDEOOUT_PARAM_SYSCAPS);
		return ret;
	}
	if ((ret = hd_videoout_close(lv->video_out_ctrl)) != HD_OK) {
		printf("hd_videoout_get:param_id(%d) fail\n", HD_VIDEOOUT_PARAM_SYSCAPS);
		return ret;
	}
	printf("LCD0: %dx%d\n", (int)lv->lcd_syscaps.input_dim.w, (int)lv->lcd_syscaps.input_dim.h);
	return ret;
}

HD_RESULT get_pb_lcd_caps(VIDEO_PLAYBACK *pb)
{
	HD_RESULT ret;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &pb->video_out_ctrl);
	if (ret != HD_OK) {
		printf("hd_videoout_get:param_id(%d) fail\n", HD_VIDEOOUT_PARAM_SYSCAPS);
		return ret;
	}
	ret = hd_videoout_get(pb->video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, &pb->lcd_syscaps);
	if (ret != HD_OK) {
		printf("hd_videoout_get:param_id(%d) fail\n", HD_VIDEOOUT_PARAM_SYSCAPS);
		return ret;
	}
	if ((ret = hd_videoout_close(pb->video_out_ctrl)) != HD_OK) {
		printf("hd_videoout_get:param_id(%d) fail\n", HD_VIDEOOUT_PARAM_SYSCAPS);
		return ret;
	}
	return ret;
}


HD_RESULT set_lv_cap_param(VIDEO_LIVEVIEW *lv, int div, int lv_tmnr)
{
	int i;
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_PATH_CONFIG cap_config;
	HD_VIDEOCAP_OUT cap_out = {0};
	
	for (i = 0; i < dvr_lv_bitstream_num; i++) {
		memset(&cap_config, 0x0, sizeof(HD_VIDEOCAP_PATH_CONFIG));
		cap_config.data_pool[0].mode = HD_VIDEOCAP_POOL_ENABLE;
		cap_config.data_pool[0].ddr_id = 0;
		cap_config.data_pool[0].counts = HD_VIDEOCAP_SET_COUNT(5, 0);
		cap_config.data_pool[1].mode = HD_VIDEOCAP_POOL_DISABLE;
		cap_config.data_pool[2].mode = HD_VIDEOCAP_POOL_DISABLE;
		cap_config.data_pool[3].mode = HD_VIDEOCAP_POOL_DISABLE;

		ret = hd_videocap_set(lv->lv_cap_list[i], HD_VIDEOCAP_PARAM_PATH_CONFIG, &cap_config);
		if (ret != HD_OK) {
			printf("hd_videocap_set HD_VIDEOCAP_PARAM_PATH_CONFIG\n");
			break;
		}

		cap_out.dim.w = lv->lcd_syscaps.input_dim.w / div;
		cap_out.dim.h = lv->lcd_syscaps.input_dim.h / div;
		cap_out.pxlfmt = HD_VIDEO_PXLFMT_YUV420_NVX3;//cap_syscaps.pxlfmt;
		cap_out.frc = HD_VIDEO_FRC_RATIO(1, 1);
		ret = hd_videocap_set(lv->lv_cap_list[i], HD_VIDEOCAP_PARAM_OUT, &cap_out);
		if (ret != HD_OK) {
			printf("hd_videocap_set HD_VIDEOCAP_PARAM_OUT\n");
			break;
		}
	}
	return ret;
}


HD_RESULT set_lv_vpe_param(VIDEO_LIVEVIEW *lv, int div)
{
	int i;
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG vpe_config;
	HD_VIDEOPROC_OUT vpe_out;
	HD_FB_FMT fb_fmt;

	fb_fmt.fb_id = HD_FB0;
	if (hd_videoout_get(lv->video_out_ctrl, HD_VIDEOOUT_PARAM_FB_FMT, &fb_fmt) != HD_OK) {
		printf("hd_videoout_get:param_id(%d) videoout_ctrl_path(%#lx) fail\n", HD_VIDEOOUT_PARAM_FB_FMT, lv->video_out_ctrl);
		return ret;
	}
	/* videoproc */
	for (i = 0; i < dvr_lv_bitstream_num; i++) {
		memset(&vpe_config, 0x0, sizeof(HD_VIDEOPROC_DEV_CONFIG));
		/* Set videoprocess out pool */
		vpe_config.data_pool[0].mode = HD_VIDEOPROC_POOL_ENABLE;
		vpe_config.data_pool[0].ddr_id = 0;
		vpe_config.data_pool[0].counts = HD_VIDEOPROC_SET_COUNT(4, 0);

		vpe_config.data_pool[1].mode = HD_VIDEOPROC_POOL_DISABLE;
		vpe_config.data_pool[2].mode = HD_VIDEOPROC_POOL_DISABLE;
		vpe_config.data_pool[3].mode = HD_VIDEOPROC_POOL_DISABLE;

		ret = hd_videoproc_set(lv->lv_vpe_list[i], HD_VIDEOPROC_PARAM_DEV_CONFIG, &vpe_config);
		if (ret != HD_OK) {
			printf("hd_videoproc_set HD_VIDEOPROC_PARAM_DEV_CONFIG\n");
			break;
		}

		vpe_out.rect.x = (i % div) * (lv->lcd_syscaps.input_dim.w / div);
		vpe_out.rect.y = (i / div) * (lv->lcd_syscaps.input_dim.h / div);
		vpe_out.rect.w = (lv->lcd_syscaps.input_dim.w / div);
		vpe_out.rect.h = (lv->lcd_syscaps.input_dim.h / div);
		vpe_out.bg.w = lv->lcd_syscaps.input_dim.w;
		vpe_out.bg.h = lv->lcd_syscaps.input_dim.h;
		vpe_out.pxlfmt = fb_fmt.fmt;
		vpe_out.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videoproc_set(lv->lv_vpe_list[i], HD_VIDEOPROC_PARAM_OUT, &vpe_out);
		if (ret != HD_OK) {
			printf("hd_videoproc_set HD_VIDEOPROC_PARAM_OUT\n");
			break;
		}
	}
	return ret;
}

HD_RESULT set_lv_lcd_param(VIDEO_LIVEVIEW *lv, int div)
{
	int i;
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_WIN_ATTR win;
	
	/* videoout */
	for (i = 0; i < dvr_lv_bitstream_num; i++) {
		win.rect.x = (i % div) * (lv->lcd_syscaps.input_dim.w / div);
		win.rect.y = (i / div) * (lv->lcd_syscaps.input_dim.h / div);
		win.rect.w = lv->lcd_syscaps.input_dim.w / div;
		win.rect.h = lv->lcd_syscaps.input_dim.h / div;

		if (i >= (div * div)) {
			win.visible = 0;
		} else {
			win.visible = 1;
		}

		ret = hd_videoout_set(lv->lv_lcd_list[i], HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &win);
		if (ret != HD_OK) {
			printf("hd_videoout_set HD_VIDEOOUT_PARAM_IN_WIN_ATTR\n");
			break;
		}
	}
	return ret;
}

HD_RESULT set_lv_tmnr_motion_buffer(VIDEO_LIVEVIEW *lv)
{
	HD_COMMON_MEM_POOL_INFO mem_info;
	mem_info.type = HD_COMMON_MEM_TMNR_MOTION_POOL;
	mem_info.ddr_id = DDR_ID0;
	if (hd_common_mem_get(HD_COMMON_MEM_PARAM_POOL_CONFIG, (VOID *)&mem_info) != HD_OK) {
		printf("Error to get HD_COMMON_MEM_TMNR_MOTION_POOL\n");				
		return HD_ERR_NG;
	}
	printf("TMNR Buffer ddr_id %d with size %dKB count %d start_addr 0x%x\n",
		mem_info.ddr_id, (int)mem_info.blk_size, (int)mem_info.blk_cnt, (int)mem_info.start_addr);

	lv->tmnr_motion_ddr_id = mem_info.ddr_id;
	lv->tmnr_motion_blk_size = mem_info.blk_size;
	lv->tmnr_motion_count = mem_info.blk_cnt;
	lv->tmnr_motion_start = (UINT32)mem_info.start_addr;
	return HD_OK;
}


HD_RESULT set_rec_cap_param(VIDEO_RECORD *rec)
{
	int i;
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_PATH_CONFIG cap_config;
	HD_VIDEOCAP_OUT cap_out = {0};

	/* videocap */
	for (i = 0; i < dvr_rec_bitstream_num; i++) {
		//set main videocap out pool
		memset(&cap_config, 0x0, sizeof(HD_VIDEOCAP_PATH_CONFIG));
		cap_config.data_pool[0].mode = HD_VIDEOCAP_POOL_ENABLE;
		cap_config.data_pool[0].ddr_id = 0;
		if (ENC_FRAME_RATE == 30) {
			cap_config.data_pool[0].counts = HD_VIDEOCAP_SET_COUNT(3, 0);
		} else {
			cap_config.data_pool[0].counts = HD_VIDEOCAP_SET_COUNT(2, 0);
		}
		cap_config.data_pool[1].mode = HD_VIDEOCAP_POOL_DISABLE;
		cap_config.data_pool[2].mode = HD_VIDEOCAP_POOL_DISABLE;
		cap_config.data_pool[3].mode = HD_VIDEOCAP_POOL_DISABLE;

		/* main */
		ret = hd_videocap_set(rec->rec_cap_list[i], HD_VIDEOCAP_PARAM_PATH_CONFIG, &cap_config);
		if (ret != HD_OK) {
			printf("hd_videocap_set for main pool fail\n");
			break;
		}

		/* sub */
		ret = hd_videocap_set(rec->rec_cap_list[i + dvr_rec_bitstream_num], HD_VIDEOCAP_PARAM_PATH_CONFIG, &cap_config);
		if (ret != HD_OK) {
			printf("hd_videocap_set for sub pool fail\n");
			break;
		}

		if (rec->enc_width[i] > rec->cap_syscaps[i].max_dim.w) {
			rec->enc_width[i] = rec->cap_syscaps[i].max_dim.w;
		}
		if (rec->enc_height[i] > rec->cap_syscaps[i].max_dim.h) {
			rec->enc_height[i] = rec->cap_syscaps[i].max_dim.h;
		}

		/* main */
		cap_out.dim.w = rec->enc_width[i];
		cap_out.dim.h = rec->enc_height[i];
		cap_out.pxlfmt = HD_VIDEO_PXLFMT_YUV420_NVX3;//cap_syscaps.pxlfmt;
		cap_out.frc = HD_VIDEO_FRC_RATIO(1, 1);
		ret = hd_videocap_set(rec->rec_cap_list[i], HD_VIDEOCAP_PARAM_OUT, &cap_out);
		if (ret != HD_OK) {
			return ret;
		}

		/* sub */
		cap_out.dim.w = rec->enc_width[i + dvr_rec_bitstream_num];
		cap_out.dim.h = rec->enc_height[i + dvr_rec_bitstream_num];
		cap_out.pxlfmt = HD_VIDEO_PXLFMT_YUV420_NVX3;//cap_syscaps.pxlfmt;
		cap_out.frc = HD_VIDEO_FRC_RATIO(1, 1);
		ret = hd_videocap_set(rec->rec_cap_list[i + dvr_rec_bitstream_num], HD_VIDEOCAP_PARAM_OUT, &cap_out);
		if (ret != HD_OK) {
			return ret;
		}
	}

	return ret;
}

#ifdef CONFIG_OSG
static HD_RESULT set_enc_stamp_param(HD_PATH_ID stamp_path_id, STAMP_INFO *p_stamp)
{
	HD_OSG_STAMP_IMG img;
	HD_OSG_STAMP_ATTR attr;
	HD_RESULT ret = HD_OK;

	img.p_addr = p_stamp->pa;
	img.dim.w = p_stamp->dim.w;
	img.dim.h = p_stamp->dim.h;
	img.fmt = HD_VIDEO_PXLFMT_ARGB1555;

	ret = hd_videoenc_set(stamp_path_id, HD_VIDEOENC_PARAM_IN_STAMP_IMG, &img);
	if (HD_OK != ret) {
		printf("Fail to set stamp image\n");
		return ret;
	}

	attr.align_type = HD_OSG_ALIGN_TYPE_TOP_LEFT;
	attr.alpha = 3;
	attr.position.x = 0;
	attr.position.y = 0;
	attr.gcac_enable = 0;
	attr.gcac_blk_width = 64;
	attr.gcac_blk_height = 32;

	ret = hd_videoenc_set(stamp_path_id, HD_VIDEOENC_PARAM_IN_STAMP_ATTR, &attr);
	if (HD_OK != ret) {
		printf("Fail to set stamp attr\n");
		return HD_ERR_SYS;
	}

	return ret;
}
#endif

HD_RESULT set_enc_param(VIDEO_RECORD *rec, STAMP_INFO *stamp)
{
	int i;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_PATH_CONFIG enc_config;
	HD_VIDEOENC_IN video_in_param;
	HD_VIDEOENC_OUT enc_param;
	HD_H26XENC_RATE_CONTROL rc_param;

	for (i = 0; i < dvr_rec_bitstream_num; i++) {
		memset(&enc_config, 0, sizeof(enc_config));
		enc_config.data_pool[0].mode = HD_VIDEOENC_POOL_ENABLE;
		enc_config.data_pool[0].ddr_id = 0;
		enc_config.data_pool[0].counts = HD_VIDEOENC_SET_COUNT(7, 0);
		enc_config.data_pool[1].mode = HD_VIDEOENC_POOL_DISABLE;
		enc_config.data_pool[2].mode = HD_VIDEOENC_POOL_DISABLE;
		enc_config.data_pool[3].mode = HD_VIDEOENC_POOL_DISABLE;

		/* main */
		ret = hd_videoenc_set(rec->rec_enc_list[i], HD_VIDEOENC_PARAM_PATH_CONFIG, &enc_config);
		if (ret != HD_OK) {
			printf("hd_videoenc_set for main stream fail\n");
			break;
		}

		/* sub */
		ret = hd_videoenc_set(rec->rec_enc_list[i + dvr_rec_bitstream_num], HD_VIDEOENC_PARAM_PATH_CONFIG, &enc_config);
		if (ret != HD_OK) {
			printf("hd_videoenc_set for main stream fail\n");
			break;
		}

		/* main */
		memset(&video_in_param, 0, sizeof(video_in_param));
		video_in_param.dim.w = rec->enc_width[i];
		video_in_param.dim.h = rec->enc_height[i];
		video_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420_NVX3;
		ret = hd_videoenc_set(rec->rec_enc_list[i], HD_VIDEOENC_PARAM_IN, &video_in_param);
		if (ret != HD_OK) {
			printf("hd_videoenc_set for main stream fail\n");
			break;
		}

#ifdef CONFIG_OSG
		if (set_enc_stamp_param(rec->rec_stamp_list[i], stamp) != HD_OK) {
			printf("set_enc_stamp_param fail\n");
			break;
		}
#endif
		/* sub */
		video_in_param.dim.w = rec->enc_width[i + dvr_rec_bitstream_num];;
		video_in_param.dim.h = rec->enc_height[i + dvr_rec_bitstream_num];
		video_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420_NVX3;
		ret = hd_videoenc_set(rec->rec_enc_list[i + dvr_rec_bitstream_num], HD_VIDEOENC_PARAM_IN, &video_in_param);
		if (ret != HD_OK) {
			printf("hd_videoenc_set for main stream fail\n");
			break;
		}
#ifdef CONFIG_OSG
		if (set_enc_stamp_param(rec->rec_stamp_list[i + dvr_rec_bitstream_num], stamp) != HD_OK) {
			printf("set_enc_stamp_param fail\n");
			break;
		}
#endif
		hd_videoenc_get(rec->rec_enc_list[i], HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &enc_param);

#ifdef CONFIG_ENCODE_H264
		enc_param.codec_type = HD_CODEC_TYPE_H264;
		enc_param.h26x.gop_num = 60; // frame_rate x (8 ~10)
		enc_param.h26x.svc_layer = HD_SVC_2X;
#else
		enc_param.codec_type = HD_CODEC_TYPE_H265;
		enc_param.h26x.gop_num = 60;
		//enc_param.h26x.ltr_interval = 30; //if not 0, need one more enc ref count
		enc_param.h26x.svc_layer = HD_SVC_2X;
		enc_param.h26x.profile = HD_H265E_MAIN_PROFILE;
		enc_param.h26x.level_idc = HD_H265E_LEVEL_4_1;
		enc_param.h26x.entropy_mode = HD_H265E_CABAC_CODING;
#endif

		/* main */
		ret = hd_videoenc_set(rec->rec_enc_list[i], HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &enc_param);
		if (ret != HD_OK) {
			printf("hd_videoenc_set for main stream fail\n");
			break;
		}

		/* sub */
		ret = hd_videoenc_set(rec->rec_enc_list[i + dvr_rec_bitstream_num], HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &enc_param);
		if (ret != HD_OK) {
			printf("hd_videoenc_set for main stream fail\n");
			break;
		}

		/* main */
		hd_videoenc_get(rec->rec_enc_list[i], HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
		rc_param.rc_mode = HD_RC_MODE_CBR;
		rc_param.cbr.frame_rate_base = ENC_FRAME_RATE; //rec->cap_syscaps[i].max_frame_rate;   //fps = 30/1 = 30
		rc_param.cbr.frame_rate_incr = 1;
		rc_param.cbr.init_i_qp = rc_param.cbr.init_p_qp = 31;
		rc_param.cbr.max_i_qp = rc_param.cbr.max_p_qp = 51;
		rc_param.cbr.min_i_qp = rc_param.cbr.min_p_qp = 15;
		rc_param.cbr.bitrate = 4096 * 1024;		//4Mbps
		ret = hd_videoenc_set(rec->rec_enc_list[i], HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
		if (ret != HD_OK) {
			printf("hd_videoenc_set for main stream fail\n");
			break;
		}

		/* sub */
		rc_param.cbr.bitrate = 512 * 1024;		//512Kbps
		ret = hd_videoenc_set(rec->rec_enc_list[i + dvr_rec_bitstream_num], HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
		if (ret != HD_OK) {
			printf("hd_videoenc_set for main stream fail\n");
			break;
		}
	}
	return ret;
}

HD_RESULT set_pb_vpe_param(VIDEO_PLAYBACK *pb, int div)
{
	int i;
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG vpe_config;
	HD_VIDEOPROC_OUT proc_out;
	HD_FB_FMT fb_fmt;

	fb_fmt.fb_id = HD_FB0;
	if (hd_videoout_get(pb->video_out_ctrl, HD_VIDEOOUT_PARAM_FB_FMT, &fb_fmt) != HD_OK) {
		printf("hd_videoout_get:HD_VIDEOOUT_PARAM_FB_FMT, fb0 fail(0x%lx)\n", pb->video_out_ctrl);
		return ret;
	}
	/* videoproc */
	memset(&vpe_config, 0x0, sizeof(HD_VIDEOPROC_DEV_CONFIG));
	for (i = 0; i < dvr_pb_bitstream_num; i++) {
		/* Set videoprocess out pool */
		vpe_config.data_pool[0].mode = HD_VIDEOPROC_POOL_ENABLE;
		vpe_config.data_pool[0].ddr_id = 0;
		vpe_config.data_pool[0].counts = HD_VIDEOPROC_SET_COUNT(3, 0);

		vpe_config.data_pool[1].mode = HD_VIDEOPROC_POOL_DISABLE;
		vpe_config.data_pool[2].mode = HD_VIDEOPROC_POOL_DISABLE;
		vpe_config.data_pool[3].mode = HD_VIDEOPROC_POOL_DISABLE;

		ret = hd_videoproc_set(pb->pb_vpe_list[i], HD_VIDEOPROC_PARAM_DEV_CONFIG, &vpe_config);
		if (ret != HD_OK) {
			printf("hd_videoproc_set HD_VIDEOPROC_PARAM_DEV_CONFIG\n");
			break;
		}

		proc_out.rect.x = (i % div) * (pb->lcd_syscaps.input_dim.w / div);
		proc_out.rect.y = (i / div) * (pb->lcd_syscaps.input_dim.h / div);
		proc_out.rect.w = pb->lcd_syscaps.input_dim.w / div;
		proc_out.rect.h = pb->lcd_syscaps.input_dim.h / div;
		proc_out.bg.w = pb->lcd_syscaps.input_dim.w;
		proc_out.bg.h = pb->lcd_syscaps.input_dim.h;
		proc_out.pxlfmt = fb_fmt.fmt;
		proc_out.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videoproc_set(pb->pb_vpe_list[i], HD_VIDEOPROC_PARAM_OUT, &proc_out);
		if (ret != HD_OK) {
			printf("hd_videoproc_set HD_VIDEOPROC_PARAM_OUT\n");
			break;
		}
	}
	return ret;
}


HD_RESULT set_dec_param(VIDEO_PLAYBACK *pb, int div, int is_h265)
{
	int i;
	HD_RESULT ret = HD_OK;
	HD_VIDEODEC_PATH_CONFIG dec_config;

	for (i = 0; i < dvr_pb_bitstream_num; i++) {
		memset(&dec_config, 0x0, sizeof(HD_VIDEODEC_PATH_CONFIG));
		/* Set videodec parameters */
		dec_config.max_mem.codec_type = (is_h265 == 1) ? HD_CODEC_TYPE_H265 : HD_CODEC_TYPE_H264;
		dec_config.max_mem.dim.w = MAINSTREAM_WIDTH;
		dec_config.max_mem.dim.h = MAINSTREAM_HEIGHT;
		dec_config.max_mem.frame_rate = 30;
		dec_config.max_mem.bs_counts = 7;
		dec_config.max_mem.max_ref_num = 1;
		dec_config.max_mem.max_bitrate = 2 * 1024 * 1024;

		/* Set videodec out pool */
		dec_config.data_pool[0].mode = HD_VIDEODEC_POOL_ENABLE;
		dec_config.data_pool[0].ddr_id = 0;
		dec_config.data_pool[0].counts = HD_VIDEODEC_SET_COUNT(3, 0);
		dec_config.data_pool[0].max_counts = HD_VIDEODEC_SET_COUNT(3, 0); //minus dec reference 1

		dec_config.data_pool[1].mode = HD_VIDEODEC_POOL_DISABLE;
		dec_config.data_pool[1].ddr_id = 0;
		dec_config.data_pool[1].counts = HD_VIDEODEC_SET_COUNT(3, 0);
		dec_config.data_pool[1].max_counts = HD_VIDEODEC_SET_COUNT(3, 0);  //minus dec reference 1

		dec_config.data_pool[2].mode = HD_VIDEODEC_POOL_DISABLE;
		dec_config.data_pool[3].mode = HD_VIDEODEC_POOL_DISABLE;

		ret = hd_videodec_set(pb->pb_dec_list[i], HD_VIDEODEC_PARAM_PATH_CONFIG, &dec_config);
		if (ret != HD_OK) {
			printf("hd_videodec_set HD_VIDEODEC_PARAM_PATH_CONFIG\n");
			break;
		}
	}
	return ret;
}

HD_RESULT set_pb_lcd_param(VIDEO_PLAYBACK *pb, int div)
{
	int i;
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_WIN_ATTR win;
	
	/* videoout */
	for (i = 0; i < dvr_pb_bitstream_num; i++) {
		win.rect.x = (i % div) * (pb->lcd_syscaps.input_dim.w / div);
		win.rect.y = (i / div) * (pb->lcd_syscaps.input_dim.h / div);
		win.rect.w = pb->lcd_syscaps.input_dim.w / div;
		win.rect.h = pb->lcd_syscaps.input_dim.h / div;

		if (i >= (div * div)) {
			win.visible = 0;
		} else {
			win.visible = 1;
		}

		ret = hd_videoout_set(pb->pb_lcd_list[i], HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &win);
		if (ret != HD_OK) {
			printf("hd_videoout_set HD_VIDEOOUT_PARAM_IN_WIN_ATTR\n");
			break;
		}
	}
	return ret;
}

HD_RESULT set_lv_tmnr_param(VIDEO_LIVEVIEW *lv, int is_tmnr)
{
	int i;
	HD_RESULT ret = HD_OK;
	VENDOR_VIDEO_PARAM_TMNR_EXT tmnr;
	VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF tmnr_motion_buf;

	for (i = 0; i < dvr_lv_bitstream_num; i++) {
		vendor_video_init(lv->lv_vpe_list[i]);

		if (is_tmnr) {
			tmnr_motion_buf.ddr_id = lv->tmnr_motion_ddr_id;
			tmnr_motion_buf.p_start_pa = (UINT32 *)(lv->tmnr_motion_start + (i * lv->tmnr_motion_blk_size));
			tmnr_motion_buf.size = lv->tmnr_motion_blk_size;
		} else {
			tmnr_motion_buf.ddr_id = 0;
			tmnr_motion_buf.p_start_pa = 0;
			tmnr_motion_buf.size = 0;
		}
		vendor_video_set(lv->lv_vpe_list[i], VENDOR_VIDEO_TMNR_MOTION_BUF, &tmnr_motion_buf);

		/* main nr */
		if (HD_OK != (ret = vendor_video_get(lv->lv_vpe_list[i], VENDOR_VIDEO_TMNR_CTRL, &tmnr))) {
			printf("Error main vendor_video_get %d\n", ret);
			goto exit_set_lv_tmnr;
		}
		tmnr.tmnr_en = is_tmnr;
		if (HD_OK != (vendor_video_set(lv->lv_vpe_list[i], VENDOR_VIDEO_TMNR_CTRL, &tmnr))) {
			printf("Error main vendor_video_set %d\n", ret);
			goto exit_set_lv_tmnr;
		}
		vendor_video_uninit(lv->lv_vpe_list[i]);
	}
exit_set_lv_tmnr:
	return ret;
}

HD_RESULT set_rec_tmnr_param(VIDEO_RECORD *rec, int is_tmnr)
{
	int i;
	HD_RESULT ret = HD_OK;
	VENDOR_VIDEO_PARAM_TMNR_EXT tmnr;

	vendor_video_init(rec->rec_enc_list[0]);
	for (i = 0; i < dvr_rec_bitstream_num; i++) {
		/* main nr */
		if ((ret = vendor_video_get(rec->rec_enc_list[i], VENDOR_VIDEO_TMNR_CTRL, &tmnr)) != HD_OK) {
			printf("Error main vendor_video_get %d\n", ret);
			goto exit_set_rec_tmnr;
		}
		tmnr.tmnr_en = is_tmnr;
		if ((vendor_video_set(rec->rec_enc_list[i], VENDOR_VIDEO_TMNR_CTRL, &tmnr)) != HD_OK) {
			printf("Error main vendor_video_set %d\n", ret);
			goto exit_set_rec_tmnr;
		}

		/* sub nr */
		if ((ret = vendor_video_get(rec->rec_enc_list[i + dvr_rec_bitstream_num], VENDOR_VIDEO_TMNR_CTRL, &tmnr)) != HD_OK) {
			printf("Error sub vendor_video_get %d\n", ret);
			goto exit_set_rec_tmnr;
		}
		tmnr.tmnr_en = is_tmnr;
		if ((vendor_video_set(rec->rec_enc_list[i + dvr_rec_bitstream_num], VENDOR_VIDEO_TMNR_CTRL, &tmnr)) != HD_OK) {
			printf("Error sub vendor_video_set %d\n", ret);
			goto exit_set_rec_tmnr;
		}
	}
	vendor_video_uninit(rec->rec_enc_list[0]);

exit_set_rec_tmnr:
	return ret;
}

HD_RESULT lv_start(VIDEO_LIVEVIEW *lv)
{
	int i;
	UINT32 ret = HD_OK;

	if (lv->is_liveview == 0) {
		for (i = 0; i < dvr_lv_bitstream_num; i++) {
			if ((ret = hd_videocap_bind(HD_VIDEOCAP_OUT(i, 0), HD_VIDEOPROC_IN(i, 0))) != HD_OK) {
				return ret;
			}
			if ((ret = hd_videoproc_bind(HD_VIDEOPROC_OUT(i, 0), HD_VIDEOOUT_IN(0, i))) != HD_OK) {
				return ret;
			}
		}

		if ((ret = hd_videocap_start_list(lv->lv_cap_list, dvr_lv_bitstream_num)) != HD_OK) {
			printf("Error hd_videodec_start_list lv_cap_list %lu\n", ret);
			ret = HD_ERR_NG;
			goto exit_lv_start;
		}
		if ((ret = hd_videoproc_start_list(lv->lv_vpe_list, dvr_lv_bitstream_num)) != HD_OK) {
			printf("Error hd_videodec_start_list lv_vpe_list %lu\n", ret);
			ret = HD_ERR_NG;
			goto exit_lv_start;
		}
		if ((ret = hd_videoout_start_list(lv->lv_lcd_list, dvr_lv_bitstream_num)) != HD_OK) {
			printf("Error hd_videodec_start_list lv_lcd_list %lu\n", ret);
			ret = HD_ERR_NG;
			goto exit_lv_start;
		}
	} else { /* for restart liveview, only start the videoout list */
		if ((ret = hd_videoout_start_list(lv->lv_lcd_list, dvr_lv_bitstream_num)) != HD_OK) {
			printf("Error hd_videodec_start_list lv_lcd_list %lu\n", ret);
			ret = HD_ERR_NG;
			goto exit_lv_start;
		}
	}

	lv->is_liveview = 1;

exit_lv_start:
	return ret;
}

HD_RESULT lv_stop(VIDEO_LIVEVIEW *lv)
{
	int i;
	HD_RESULT ret = HD_OK;

	if (lv->is_liveview == 0) {
		return ret;
	}

	lv->is_liveview = 0;
	if ((ret = hd_videocap_stop_list(lv->lv_cap_list, dvr_lv_bitstream_num)) != HD_OK) {
		printf("Error hd_videocap_stop_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_lv_stop;
	}

	if ((ret = hd_videoproc_stop_list(lv->lv_vpe_list, dvr_lv_bitstream_num)) != HD_OK) {
		printf("Error hd_videoproc_stop_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_lv_stop;
	}

	if ((ret = hd_videoout_stop_list(lv->lv_lcd_list, dvr_lv_bitstream_num)) != HD_OK) {
		printf("Error hd_videoout_stop_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_lv_stop;
	}

	for (i = 0; i < dvr_lv_bitstream_num; i++) {
		if ((ret = hd_videocap_unbind(HD_VIDEOCAP_OUT(i, 0))) != HD_OK) {
			printf("Error hd_videocap_unbind %d\n", ret);
			ret = HD_ERR_NG;
			break;
		}
		if ((ret = hd_videoproc_unbind(HD_VIDEOPROC_OUT(i, 0))) != HD_OK) {
			printf("Error hd_videoproc_unbind %d\n", ret);
			ret = HD_ERR_NG;
			break;
		}
	}
exit_lv_stop:
	return ret;
}

HD_RESULT rec_start(VIDEO_RECORD *rec)
{
	int i;
	HD_RESULT ret = HD_OK;

	for (i = 0; i < dvr_rec_bitstream_num; i++) {
		/* main */
		if ((ret = hd_videocap_bind(HD_VIDEOCAP_OUT(i, 1), HD_VIDEOENC_IN(0, i))) != HD_OK) {
			printf("Error hd_videocap_bind %d\n", ret);
			return ret;
		}
		/* sub */
		if ((ret = hd_videocap_bind(HD_VIDEOCAP_OUT(i, 2), HD_VIDEOENC_IN(0, i + dvr_rec_bitstream_num))) != HD_OK) {
			printf("Error hd_videocap_bind %d\n", ret);
			return ret;
		}
		if ((ret = hd_videoenc_start(rec->rec_stamp_list[i])) != HD_OK) {
			printf("Error hd_videoenc_start rec_stamp_list %d\n", ret);
			return ret;
		}
		if ((ret = hd_videoenc_start(rec->rec_stamp_list[i + dvr_rec_bitstream_num])) != HD_OK) {
			printf("Error hd_videoenc_start rec_stamp_list %d\n", ret);
			return ret;
		}
	}

	if ((ret = hd_videocap_start_list(rec->rec_cap_list, dvr_rec_bitstream_num * 2)) != HD_OK) {
		printf("Error hd_videodec_start_list rec_cap_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_rec_start;
	}
	if ((ret = hd_videoenc_start_list(rec->rec_enc_list, dvr_rec_bitstream_num * 2)) != HD_OK) {
		printf("Error hd_videodec_start_list rec_enc_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_rec_start;
	}
	if (pthread_create(&rec->record_thread_id, NULL, record_thread, (void *)rec) < 0) {
		printf("create thread record_thread failed!");
		ret = HD_ERR_NG;
		goto exit_rec_start;
	}
	pthread_setname_np(rec->record_thread_id,"rec");
	rec->is_record = 1;

exit_rec_start:
	return ret;
}

HD_RESULT rec_stop(VIDEO_RECORD *rec)
{
	int i;
	HD_RESULT ret = HD_OK;

	if (rec->is_record == 0) {
		return HD_OK;
	}
	rec->is_record = 0;

	pthread_join(rec->record_thread_id, NULL);

	if ((ret = hd_videocap_stop_list(rec->rec_cap_list, dvr_rec_bitstream_num * 2)) != HD_OK) {
		printf("Error hd_videocap_stop_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_rec_stop;
	}

	if ((ret = hd_videoenc_stop_list(rec->rec_enc_list, dvr_rec_bitstream_num * 2)) != HD_OK) {
		printf("Error hd_videoproc_stop_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_rec_stop;
	}

	for (i = 0; i < dvr_rec_bitstream_num; i++) {
		if ((ret = hd_videoenc_stop(rec->rec_stamp_list[i])) != HD_OK) {
			printf("Error hd_videocap_unbind %d\n", ret);
			ret = HD_ERR_NG;
			break;
		}
		if ((ret = hd_videoenc_stop(rec->rec_stamp_list[i + dvr_rec_bitstream_num])) != HD_OK) {
			printf("Error hd_videocap_unbind %d\n", ret);
			ret = HD_ERR_NG;
			break;
		}		
		if ((ret = hd_videocap_unbind(HD_VIDEOCAP_OUT(i, 1))) != HD_OK) {
			printf("Error hd_videocap_unbind %d\n", ret);
			ret = HD_ERR_NG;
			break;
		}
		if ((ret = hd_videocap_unbind(HD_VIDEOCAP_OUT(i, 2))) != HD_OK) {
			printf("Error hd_videocap_unbind %d\n", ret);
			ret = HD_ERR_NG;
			break;
		}
	}
exit_rec_stop:
	return ret;
}

HD_RESULT pb_start(VIDEO_PLAYBACK *pb)
{
	int i;
	UINT32 ret = HD_OK;

	if (pb->is_playback == 0) {
		for (i = 0; i < dvr_pb_bitstream_num; i++) {
			if ((ret = hd_videodec_bind(HD_VIDEODEC_OUT(0, i), HD_VIDEOPROC_IN(i + dvr_lv_bitstream_num, 0))) != HD_OK) {
				printf("Error hd_videodec_unbind %lu\n", ret);
				return ret;
			}
			if ((ret = hd_videoproc_bind(HD_VIDEOPROC_OUT(i + dvr_lv_bitstream_num, 0), HD_VIDEOOUT_IN(0, i + dvr_lv_bitstream_num))) != HD_OK) {
				printf("Error hd_videoproc_unbind %lu\n", ret);
				return ret;
			}
		}

		if ((ret = hd_videodec_start_list(pb->pb_dec_list, dvr_pb_bitstream_num)) != HD_OK) {
			printf("Error hd_videodec_start_list pb_dec_list %lu\n", ret);
			ret = HD_ERR_NG;
			goto exit_pb_start;
		}

		if ((ret = hd_videoproc_start_list(pb->pb_vpe_list, dvr_pb_bitstream_num)) != HD_OK) {
			printf("Error hd_videodec_start_list pb_vpe_list %lu\n", ret);
			ret = HD_ERR_NG;
			goto exit_pb_start;
		}
		if ((ret = hd_videoout_start_list(pb->pb_lcd_list, dvr_pb_bitstream_num)) != HD_OK) {
			printf("Error hd_videodec_start_list pb_lcd_list %lu\n", ret);
			ret = HD_ERR_NG;
			goto exit_pb_start;
		}
	} else { /* for restart playback, only start the videoout list */
		if ((ret = hd_videoout_start_list(pb->pb_lcd_list, dvr_pb_bitstream_num)) != HD_OK) {
			printf("Error hd_videodec_start_list pb_lcd_list %lu\n", ret);
			ret = HD_ERR_NG;
			goto exit_pb_start;
		}
	}

	if (pb->is_playback == 0) {
		pb->is_playback = 1;

		if (pthread_create(&pb->playback_thread_id, NULL, playback_thread, (void *)pb) < 0) {
			printf("create thread playback_thread failed!");
			ret = HD_ERR_NG;
			goto exit_pb_start;
		}
		pthread_setname_np(pb->playback_thread_id,"pb");
	}
	
exit_pb_start:
	return ret;
}

HD_RESULT pb_stop(VIDEO_PLAYBACK *pb)
{
	int i;
	HD_RESULT ret = HD_OK;

	if (pb->is_playback == 0) {
		return 0;
	}
	pb->is_playback = 0;

	pthread_join(pb->playback_thread_id, NULL);

	if ((ret = hd_videodec_stop_list(pb->pb_dec_list, dvr_pb_bitstream_num)) != HD_OK) {
		printf("Error hd_videodec_stop_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_pb_stop;
	}
	if ((ret = hd_videoproc_stop_list(pb->pb_vpe_list, dvr_pb_bitstream_num)) != HD_OK) {
		printf("Error hd_videoproc_stop_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_pb_stop;
	}
	if ((ret = hd_videoout_stop_list(pb->pb_lcd_list, dvr_pb_bitstream_num)) != HD_OK) {
		printf("Error hd_videoout_stop_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_pb_stop;
	}

	for (i = 0; i < dvr_pb_bitstream_num; i++) {
		if ((ret = hd_videodec_unbind(HD_VIDEODEC_OUT(0, i))) != HD_OK) {
			printf("Error hd_videodec_unbind %d\n", ret);
			break;
		}
		if ((ret = hd_videoproc_unbind(HD_VIDEOPROC_OUT(i + dvr_lv_bitstream_num, 0))) != HD_OK) {
			printf("Error hd_videoproc_unbind %d\n", ret);
			break;
		}
	}
exit_pb_stop:
	pthread_join(pb->playback_thread_id, NULL);
	return ret;
}


static HD_RESULT init_stamp_buffer(STAMP_INFO *p_stamp)
{
	INT32 size;
	void *p_va;
	HD_RESULT ret = HD_OK;
	FILE *stamp_fd;

	size = p_stamp->dim.w * p_stamp->dim.h * 2;
	if (size < 0) {
		return HD_ERR_NG;
	}

	//get osd stamps' block
	p_stamp->blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, size, DDR_ID0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == p_stamp->blk) {
		printf("get block fail\r\n");
		return HD_ERR_NG;
	}

	//translate stamp block to physical address
	p_stamp->pa = hd_common_mem_blk2pa(p_stamp->blk);
	if (0 == p_stamp->pa) {
		printf("blk2pa fail, blk = 0x%lx\r\n", p_stamp->blk);
		hd_common_mem_release_block(p_stamp->blk);
		return HD_ERR_NG;
	}

	p_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, p_stamp->pa, size);
	if (NULL == p_va) {
		printf("hd_common_mem_mmap fail\n");
		return HD_ERR_NG;
	}
	memset(p_va, 0, size);

	printf("Load OSG pattern: %s\n", p_stamp->name);
	
	if ((stamp_fd = fopen(p_stamp->name, "rb+")) == NULL) {
		printf("[ERROR] Open %s failed!!\n", p_stamp->name);
		goto exit_init_buf;
	}	
	fread(p_va, 1, size, stamp_fd);
	fclose(stamp_fd);

exit_init_buf:
	if (HD_OK != hd_common_mem_munmap(p_va, size)) {
		printf("hd_common_mem_munmap fail\n");
		ret = HD_ERR_NG;
	}
	return ret;
}

HD_RESULT uninit_stamp_buffer(STAMP_INFO *p_stamp)
{
	if (HD_COMMON_MEM_VB_INVALID_BLK != p_stamp->blk) {
		hd_common_mem_release_block(p_stamp->blk);
		p_stamp->blk = HD_COMMON_MEM_VB_INVALID_BLK;
	}
	p_stamp->blk = HD_COMMON_MEM_VB_INVALID_BLK;
	return HD_OK;
}


HD_RESULT init_module(void)
{
	UINT32 ret = HD_OK;
	if ((ret = hd_common_init(1)) != HD_OK) {
		printf("common init fail\n");
		ret = HD_ERR_NG;
		goto init_return;
	}
	if ((ret = hd_videocap_init()) != HD_OK) {
		printf("Error hd_videocap_init %lu\n", ret);
		ret = HD_ERR_NG;
		goto init_return;
	}
	if ((ret = hd_videoproc_init()) != HD_OK) {
		printf("Error hd_videoproc_init %lu\n", ret);
		ret = HD_ERR_NG;
		goto init_return;
	}
	if ((ret = hd_videoout_init()) != HD_OK) {
		printf("Error hd_videoout_init %lu\n", ret);
		ret = HD_ERR_NG;
		goto init_return;
	}
	if ((ret = hd_videoenc_init()) != HD_OK) {
		printf("Error hd_videoenc_init %lu\n", ret);
		ret = HD_ERR_NG;
		goto init_return;
	}
	if ((ret = hd_videodec_init()) != HD_OK) {
		printf("Error hd_videodec_init %lu\n", ret);
		ret = HD_ERR_NG;
		goto init_return;
	}
init_return:
	return ret;
}

HD_RESULT open_module(VIDEO_LIVEVIEW *lv, VIDEO_RECORD *rec, VIDEO_PLAYBACK *pb)
{
	int i;
	UINT32 ret = HD_OK;
		
	for (i = 0; i < dvr_lv_bitstream_num; i++) {
		/* videocap: liveview */
		if ((ret = hd_videocap_open(HD_VIDEOCAP_IN(i, 0), HD_VIDEOCAP_OUT(i, 0), &lv->lv_cap_list[i])) != HD_OK) {
			printf("Error hd_videocap_open %lu\n", ret);
			goto exit_open_module;
		}		
		/* videoprocess: liveview */
		if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(i, 0), HD_VIDEOPROC_OUT(i, 0), &lv->lv_vpe_list[i])) != HD_OK) {
			printf("Error hd_videoproc_open %lu\n", ret);
			goto exit_open_module;
		}

		/* videoput: liveview */
	    if ((ret = hd_videoout_open(HD_VIDEOOUT_IN(0, i), HD_VIDEOOUT_OUT(0, 0), &lv->lv_lcd_list[i])) != HD_OK) {
			printf("Error hd_videoout_open %lu\n", ret);
			goto exit_open_module;
		}
	}

	for (i = 0; i < dvr_pb_bitstream_num; i++) {
		/* videodec: playback */
		if ((ret = hd_videodec_open(HD_VIDEODEC_IN(0, i), HD_VIDEODEC_OUT(0, i), &pb->pb_dec_list[i])) != HD_OK) {
			printf("Error hd_videodec_open %lu\n", ret);
			goto exit_open_module;
		}

		/* videoprocess: playback */
		if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(i + dvr_lv_bitstream_num, 0), HD_VIDEOPROC_OUT(i + dvr_lv_bitstream_num, 0),
				&pb->pb_vpe_list[i])) != HD_OK) {
			printf("Error hd_videoproc_open %lu\n", ret);
			goto exit_open_module;
		}

		/* videoput: playback */
	    if ((ret = hd_videoout_open(HD_VIDEOOUT_IN(0, i + dvr_lv_bitstream_num), HD_VIDEOOUT_OUT(0, 0), &pb->pb_lcd_list[i])) != HD_OK) {
			printf("Error hd_videoout_open %lu\n", ret);
			goto exit_open_module;
		}
	}

	for (i = 0; i < dvr_rec_bitstream_num; i++) {
		/* videocap: record mainstream */
		if ((ret = hd_videocap_open(HD_VIDEOCAP_IN(i, 0), HD_VIDEOCAP_OUT(i, 1), &rec->rec_cap_list[i])) != HD_OK) {
			printf("Error hd_videocap_open %lu\n", ret);
			goto exit_open_module;
		}
		/* videoenc: mainstream */
		if ((ret = hd_videoenc_open(HD_VIDEOENC_IN(0, i), HD_VIDEOENC_OUT(0, i), &rec->rec_enc_list[i])) != HD_OK) {
			printf("Error hd_videoenc_open %lu\n", ret);
			goto exit_open_module;
		}

		if ((ret = hd_videoenc_open(HD_VIDEOENC_IN(0, HD_STAMP(0)), HD_VIDEOENC_OUT(0, i), &rec->rec_stamp_list[i])) != HD_OK) {
			printf("Error hd_videoenc_open %lu\n", ret);
			return ret;
		}

		/* videocap: record substream */
		if ((ret = hd_videocap_open(HD_VIDEOCAP_IN(i, 0), HD_VIDEOCAP_OUT(i, 2), &rec->rec_cap_list[i + dvr_rec_bitstream_num])) != HD_OK) {
			printf("Error hd_videocap_open %lu\n", ret);
			goto exit_open_module;
		}
		/* videoenc: substream */
		if ((ret = hd_videoenc_open(HD_VIDEOENC_IN(0, i + dvr_rec_bitstream_num),
								    HD_VIDEOENC_OUT(0, i + dvr_rec_bitstream_num),
								    &rec->rec_enc_list[i + dvr_rec_bitstream_num])) != HD_OK) {
			printf("Error hd_videoenc_open %lu\n", ret);
			goto exit_open_module;
		}
		if ((ret = hd_videoenc_open(HD_VIDEOENC_IN(0, HD_STAMP(0)),
									HD_VIDEOENC_OUT(0, i + dvr_rec_bitstream_num),
									&rec->rec_stamp_list[i + dvr_rec_bitstream_num])) != HD_OK) {
			printf("Error hd_videoenc_open %lu\n", ret);
			return ret;
		}
	}
exit_open_module:
	return ret;
}


HD_RESULT close_module(VIDEO_LIVEVIEW *lv, VIDEO_RECORD *rec, VIDEO_PLAYBACK *pb)
{
	int i;
	HD_RESULT ret = HD_OK;

	/* clear tmnr motion buffer */
	set_rec_tmnr_param(rec, 0);
	set_lv_tmnr_param(lv, 0);
	
	for (i = 0; i < dvr_lv_bitstream_num; i++) {
		if ((ret = hd_videocap_close(lv->lv_cap_list[i])) != HD_OK) {
			printf("Error hd_videocap_close %d\n", ret);
			break;
		}

	    if ((ret = hd_videoproc_close(lv->lv_vpe_list[i])) != HD_OK) {
			printf("Error hd_videoproc_close %d\n", ret);
			break;
		}
	    if ((ret = hd_videoout_close(lv->lv_lcd_list[i])) != HD_OK) {
			printf("Error hd_videoproc_close %d\n", ret);
			break;
		}
	}

	for (i = 0; i < dvr_pb_bitstream_num; i++) {
		if ((ret = hd_videodec_close(pb->pb_dec_list[i])) != HD_OK) {
			printf("Error hd_videodec_close %d\n", ret);
			break;
		}
	    if ((ret = hd_videoproc_close(pb->pb_vpe_list[i])) != HD_OK) {
			printf("Error hd_videoproc_close %d\n", ret);
			break;
		}
	    if ((ret = hd_videoout_close(pb->pb_lcd_list[i])) != HD_OK) {
			printf("Error hd_videoproc_close %d\n", ret);
			break;
		}
	}

	for (i = 0; i < dvr_rec_bitstream_num; i++) {
		if ((ret = hd_videocap_close(rec->rec_cap_list[i])) != HD_OK) {
			printf("Error hd_videocap_close %d\n", ret);
			break;
		}
	    if ((ret = hd_videocap_close(rec->rec_cap_list[i + dvr_rec_bitstream_num])) != HD_OK) {
			printf("Error hd_videocap_close %d\n", ret);
			break;
		}
		if ((ret = hd_videoenc_close(rec->rec_stamp_list[i])) != HD_OK) {
			printf("Error hd_videoenc_close %d\n", ret);
			break;
		}
		if ((ret = hd_videoenc_close(rec->rec_stamp_list[i + dvr_rec_bitstream_num])) != HD_OK) {
			printf("Error hd_videoenc_close %d\n", ret);
			break;
		}
		
		if ((ret = hd_videoenc_close(rec->rec_enc_list[i])) != HD_OK) {
			printf("Error hd_videoenc_close %d\n", ret);
			break;
		}
	    if ((ret = hd_videoenc_close(rec->rec_enc_list[i + dvr_rec_bitstream_num])) != HD_OK) {
			printf("Error hd_videoenc_close %d\n", ret);
			break;
		}
		
	}
	return ret;
}

HD_RESULT exit_module(void)
{
	int ret = HD_OK;
	if ((ret = hd_videocap_uninit()) != HD_OK) {
		printf("Error hd_videocap_uninit %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_return;
	}
	if ((ret = hd_videoproc_uninit()) != HD_OK) {
		printf("Error hd_videoprocess_uninit %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_return;
	}
	if ((ret = hd_videoout_uninit()) != HD_OK) {
		printf("Error hd_videoout_uninit %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_return;
	}
	if ((ret = hd_videoenc_uninit()) != HD_OK) {
		printf("Error hd_videoenc_uninit %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_return;
	}
	if ((ret = hd_videodec_uninit()) != HD_OK) {
		printf("Error hd_videodec_uninit %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_return;
	}
	if ((ret = hd_common_uninit()) != HD_OK) {
		printf("common uninit fail\n");
		ret = HD_ERR_NG;
		goto exit_return;
	}
exit_return:
	return ret;
}


unsigned int time_to_ms(struct timeval *a)
{
	return (a->tv_sec * 1000) + (a->tv_usec / 1000);
}

static void *playback_thread(void *arg_pb)
{
	VIDEO_PLAYBACK *pb = (VIDEO_PLAYBACK *)arg_pb;
	int ret = 0, length = 0, ch = 0, seq_num = 0, ts_info = 0;
	FILE *bs_fd, *len_fd;
	char *playback_buffer;
	unsigned int frame_period;
	unsigned int channel_time;
	HD_VIDEODEC_SEND_LIST video_bs[dvr_pb_bitstream_num];
	unsigned int perf_start_ms = 0, frame_counts = 0, now_time_ms = 0;
	struct timeval time_start;
	unsigned int sleep_ms = 0;

	memset(video_bs, 0, sizeof(video_bs));
	frame_period = 1000 / 30;   //frame rate 30fps
	if ((bs_fd = fopen(PATTERN_BS_FILE, "rb+")) == NULL) {
		printf("[ERROR] CH%d Open %s failed!!\n", ch, PATTERN_BS_FILE);
		exit(1);
	}
	printf("Playback %s frame_period %dms\n", PATTERN_BS_FILE, frame_period);
	if ((len_fd = fopen(PATTERN_LEN_FILE, "rb+")) == NULL) {
		printf("[ERROR] CH%d Open %s failed!!\n", ch, PATTERN_LEN_FILE);
		exit(1);
	}
	playback_buffer = malloc(MAINSTREAM_LEN);
	if (playback_buffer == NULL) {
		perror("Error allocation bitstream buffer\n");
	}

	gettimeofday(&time_start, NULL);
	perf_start_ms = time_to_ms(&time_start);
	channel_time = perf_start_ms + frame_period;

	while (pb->is_playback) {
		if (fscanf(len_fd, "%d %d %d\n", &seq_num, &ts_info, &length) == EOF) {
			fseek(bs_fd, 0, SEEK_SET);
			fseek(len_fd, 0, SEEK_SET);
			fscanf(len_fd, "%d %d %d\n", &seq_num, &ts_info, &length);
		}
		if (length == 0) {
			break;
		}
		fread(playback_buffer, 1, length, bs_fd);

		for (ch = 0; ch < dvr_pb_bitstream_num; ch++) {
			video_bs[ch].path_id = pb->pb_dec_list[ch];
			video_bs[ch].user_bs.p_bs_buf = playback_buffer;
			video_bs[ch].user_bs.bs_buf_size = length;
		}

		gettimeofday(&time_start, NULL);
		now_time_ms = time_to_ms(&time_start);

		if ((now_time_ms - perf_start_ms) > INTERVAL_TIME_MS) {
			printf("Playback %d frames in %dms (each %dfps)\n", frame_counts, now_time_ms -
				   perf_start_ms, (frame_counts * 1000) / (now_time_ms - perf_start_ms));
			frame_counts = 0;
			perf_start_ms = now_time_ms;
		}

		sleep_ms = 0;
		if (channel_time > now_time_ms) {
			sleep_ms = channel_time - now_time_ms;
		}
		if (sleep_ms) {
			usleep(sleep_ms * 1000);    //no data playback
		}
		while ((pb->is_playback) && ((ret = hd_videodec_send_list(video_bs, dvr_pb_bitstream_num, 500)) < 0)) {
			usleep(500 * 1000);
			printf("<send bitstream fail(%d)!>\n", ret);
			continue;
		}
		channel_time += frame_period;
		frame_counts++;
	}

	fclose(bs_fd);
	fclose(len_fd);
	free(playback_buffer);
	return 0;
}

static void *record_thread(void *arg_rec)
{
	VIDEO_RECORD *rec = (VIDEO_RECORD *)arg_rec;
	int i, j, ret, poll_num = dvr_rec_bitstream_num * 2;
	struct timeval time_start, poll_start;
	unsigned int perf_start_ms = 0, now_time_ms = 0, poll_time_ms = 0, sleep_ms = 0;
	unsigned int min_frame_period = 1000 / 30, total_time_ms = 0, total_main = 0, total_sub = 0;
	unsigned int frame_counts[MAX_REC_BITSTREAM_NUM * 2];
	CHAR *bitstream_data[MAX_REC_BITSTREAM_NUM * 2];
	unsigned int bs_len[MAX_REC_BITSTREAM_NUM * 2];
	HD_VIDEOENC_POLL_LIST poll_list[MAX_REC_BITSTREAM_NUM * 2];
	HD_VIDEOENC_RECV_LIST recv_list[MAX_REC_BITSTREAM_NUM * 2];

	memset(poll_list, 0, sizeof(poll_list));
	for (i = 0; i < poll_num; i++) {
		bitstream_data[i] = (char *)malloc(MAINSTREAM_LEN);
		if (bitstream_data[i] == 0) {
			return 0;
		}
		memset(bitstream_data[i], 0, MAINSTREAM_LEN);
		poll_list[i].path_id = rec->rec_enc_list[i];
		frame_counts[i] = 0;
	}

	gettimeofday(&time_start, NULL);
	poll_time_ms = perf_start_ms = time_to_ms(&time_start);

	while (rec->is_record) {
		gettimeofday(&poll_start, NULL);

		if (min_frame_period > (time_to_ms(&poll_start) - poll_time_ms)) {
			sleep_ms = min_frame_period - (time_to_ms(&poll_start) - poll_time_ms);
			usleep((sleep_ms * 1000) / 2);
		}

		poll_time_ms = time_to_ms(&poll_start);

		ret = hd_videoenc_poll_list(poll_list, poll_num, 500);
		if (ret == HD_ERR_TIMEDOUT) {
			printf("Poll timeout!!");
			continue;
		}

		memset(recv_list, 0, sizeof(recv_list));
		for (i = 0; i < poll_num; i++) {
			if (poll_list[i].revent.event != TRUE) {
				continue;
			}
			if (poll_list[i].revent.bs_size > MAINSTREAM_LEN) {
				printf("bitstream buffer bs_size is not enough! %lu, %d\n",
						poll_list[i].revent.bs_size, MAINSTREAM_LEN);
				continue;
			}
			recv_list[i].path_id = poll_list[i].path_id;
			recv_list[i].user_bs.p_user_buf = bitstream_data[i];
			recv_list[i].user_bs.user_buf_size = MAINSTREAM_LEN;
		}

		if ((ret = hd_videoenc_recv_list(recv_list, poll_num)) < 0) {
			printf("Error return value %d\n", ret);
			continue;
		}

		for (i = 0; i < poll_num ; i++) {
			unsigned int bs_size = 0;
			if (recv_list[i].retval >= 0) {
				for (j = 0; j < recv_list[i].user_bs.pack_num; j++) {
					bs_size += recv_list[i].user_bs.video_pack[j].size;
				}
				frame_counts[i]++;
				bs_len[i] += bs_size;
			}
		}

		gettimeofday(&time_start, NULL);
		now_time_ms = time_to_ms(&time_start);
		total_time_ms = now_time_ms - perf_start_ms;
		if (total_time_ms > INTERVAL_TIME_MS) {
			for (i = 0; i < poll_num; i++) {
				if (i < dvr_rec_bitstream_num) {
					total_main += frame_counts[i];
				} else {
					total_sub += frame_counts[i];
				}

				printf("Record CH%2d %dx%d %d.%02dfps %dkbps\n", i,
						rec->enc_width[i], rec->enc_height[i],  (frame_counts[i] * 1000 / total_time_ms),
					   (frame_counts[i] * 100000 / total_time_ms) % 100,
					   (bs_len[i] * 8 / 1024) * 1000 / total_time_ms);
				frame_counts[i] = 0;
				bs_len[i] = 0;
			}
			printf("Record MAIN Total %2dCH %d.%02dfps\n", dvr_rec_bitstream_num,
				   (total_main * 1000 / total_time_ms) / dvr_rec_bitstream_num,
				   (total_main * 100000 / total_time_ms) / dvr_rec_bitstream_num % 100);
			printf("Record SUB  Total %2dCH %d.%02dfps\n", dvr_rec_bitstream_num,
				   (total_sub * 1000 / total_time_ms) / dvr_rec_bitstream_num,
				   (total_sub * 100000 / total_time_ms) / dvr_rec_bitstream_num % 100);
			perf_start_ms = now_time_ms;
			total_main = total_sub = 0;
			printf("\n");
		}
	}

	for (i = 0; i < poll_num; i++) {
		free(bitstream_data[i]);
	}
	return 0;
}

void show_menu(void)
{
	printf("Usage:\r\n#./dvr_perf [LV tmnr:0/1] [ENC tmnr:0/1]\n");
	printf(" (L)Liveview\n");
	printf(" (P)Playback\n");
	printf(" (R)Record\n");
	printf(" (D)Debug Menu\n");
	printf(" (Q)Exit\n");
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	int i, key, is_exit = 0, lv_div = 0, pb_div = 0, lv_tmnr = 1, rec_tmnr = 1, is_h265 = 1;
	int run_inputs = dvr_rec_bitstream_num;
	STAMP_INFO stamps = {"osg_88x32_rgb1555.bin", {88, 32}, HD_COMMON_MEM_VB_INVALID_BLK, 0};
	//STAMP_INFO stamps = {"256x256_argb1555.bin", {256, 256}, HD_COMMON_MEM_VB_INVALID_BLK, 0};
	VIDEO_LIVEVIEW lv;
	VIDEO_RECORD rec;
	VIDEO_PLAYBACK pb;

#ifdef CONFIG_ENCODE_H264
	is_h265 = 0;
#endif

	if (argc == 4) {
		lv_tmnr = atoi(argv[1]);
		rec_tmnr = atoi(argv[2]);
		is_h265 = atoi(argv[3]);
	} else if (argc > 1) {
		printf("Usage: dvr_perf [lv_tmnr:0/1] [rec_tmnr:0/1] [is_h265:0/1]\n");
		goto exit_main;
	}

	printf("dvr_perf %s Liveview TMNR(%d), Encode TMNR(%d), Decode(%s)\n",
		DVR_PERF_VERSION, lv_tmnr, rec_tmnr, (is_h265 == 1)? "H265" : "AUTO");

	lv.is_liveview = pb.is_playback = rec.is_record = 0;	
	if (init_module() != HD_OK) {
		printf("Error init_module!\n");
		goto exit_main;
	}
	if (open_module(&lv, &rec, &pb) != HD_OK) {
		printf("Error open_module!\n");
		goto exit_main;
	}

	if (init_stamp_buffer(&stamps) != HD_OK) {
		printf("init stamps[0] fail\n");
		goto exit_main;
	}

	set_lv_tmnr_motion_buffer(&lv);

	if (get_lv_cap_caps(&lv) != HD_OK) {
		printf("Error get_lv_cap_caps!\n");
		goto exit_main;
	}
	if (get_rec_cap_caps(&rec) != HD_OK) {
		printf("Error get_rec_cap_caps!\n");
		goto exit_main;
	}
	if (get_lv_lcd_caps(&lv) != HD_OK) {
		printf("Error get_lv_lcd_caps!\n");
		goto exit_main;
	}
	if (get_pb_lcd_caps(&pb) != HD_OK) {
		printf("Error get_pb_lcd_caps!\n");
		goto exit_main;
	}
	set_rec_tmnr_param(&rec, rec_tmnr);
	set_lv_tmnr_param(&lv, lv_tmnr);

	for (i = 0; i < dvr_rec_bitstream_num; i++) {
		rec.enc_width[i] = MAINSTREAM_WIDTH;
		rec.enc_height[i] = MAINSTREAM_HEIGHT;
		rec.enc_width[i + dvr_rec_bitstream_num] = SUBSTREAM_WIDTH;
		rec.enc_height[i + dvr_rec_bitstream_num] = SUBSTREAM_HEIGHT;
	}

	show_menu();
	while (is_exit == 0) {

		key = getchar();
		switch (key) {
		case 'L': case 'l':
			if (lv_div > 1) {
				lv_div--;
			} else if (run_inputs == 8) { //8ch
				lv_div = 3;
			} else if (run_inputs == 4) { //4ch
				lv_div = 2;
			}
			printf("Start Liveview (div%d)...\n", lv_div * lv_div);
			pb_stop(&pb);
			pb_div = 0;
			set_lv_cap_param(&lv, lv_div, lv_tmnr);
			set_lv_vpe_param(&lv, lv_div);
			set_lv_lcd_param(&lv, lv_div);
			lv_start(&lv);
			break;
		case 'P': case 'p':
			if (pb_div > 1) {
				pb_div--;
			} else {
				if (dvr_pb_bitstream_num == 1) { //1ch
					pb_div = 2;
				} else if (dvr_pb_bitstream_num == 4) { //4ch
					pb_div = 2;
				} else { //8ch
					pb_div = 3;
				}
			}

			printf("Start %sPlayback (div%d)...\n", (is_h265 == 1)? "H265 " : "", pb_div * pb_div);
			lv_stop(&lv);
			lv_div = 0;
			set_pb_vpe_param(&pb, pb_div);
			set_dec_param(&pb, pb_div, is_h265);
			set_pb_lcd_param(&pb, pb_div);
			pb_start(&pb);
			break;
		case 'R': case 'r':
			if (rec.is_record == 0) {
				printf("Start Record...\n");
				set_rec_cap_param(&rec);
				set_enc_param(&rec, &stamps);
				rec_start(&rec);
			}
			break;
		case 'd': case 'D':
			hd_debug_run_menu();
			break;
		case 'Q': case 'q':
			rec_stop(&rec);
			lv_stop(&lv);
			pb_stop(&pb);
			is_exit = 1;
			break;
		default:
			show_menu();
			break;
		}
	}

	lv_stop(&lv);
	pb_stop(&pb);
	rec_stop(&rec);

	if (uninit_stamp_buffer(&stamps) != HD_OK) {
		printf("Error uninit stamp!\n");
		goto exit_main;
	}
	if (close_module(&lv, &rec, &pb) != HD_OK) {
		printf("Error close_module!\n");
		goto exit_main;
	}
	if (exit_module() != HD_OK) {
		printf("Error exit_module!\n");
	}
exit_main:
	return 0;
}
