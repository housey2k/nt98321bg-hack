/**
   @file nvr_perf.c

   @brief playback perf sample.

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

#define NVR_PERF_VERSION	"v1.0"

#define MAINSTREAM_WIDTH    1920
#define MAINSTREAM_HEIGHT   1080
#define MAINSTREAM_LEN      ((MAINSTREAM_WIDTH * MAINSTREAM_HEIGHT * 3) / 2)

#define MAX_BITSTREAM_NUM   4
#define INTERVAL_TIME_MS    5000
#define PB_FRAME_RATE       30

#define PATTERN_H264_BS_FILE	"1920x1080_ref2.264"
#define PATTERN_H264_LEN_FILE	"1920x1080_ref2.len"
#define PATTERN_H265_BS_FILE	"1920x1080_ref1.265"
#define PATTERN_H265_LEN_FILE	"1920x1080_ref1.len"

typedef struct _VIDEO_PLAYBACK {                            // video path
    HD_PATH_ID pb_dec_list[MAX_BITSTREAM_NUM];
   	HD_PATH_ID pb_lcd_list[MAX_BITSTREAM_NUM];
	HD_PATH_ID pb_vpe_list[MAX_BITSTREAM_NUM];	
	HD_PATH_ID video_out_ctrl;
	HD_VIDEOOUT_SYSCAPS lcd_syscaps;
	pthread_t playback_thread_id;
	CHAR pattern_bs_file[32];
	CHAR pattern_len_file[32];
	INT is_playback;
	INT is_h265;
	UINT fps;
} VIDEO_PLAYBACK;

static void *playback_thread(void *arg_pb);

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
	for (i = 0; i < MAX_BITSTREAM_NUM; i++) {
		/* Set videoprocess out pool */
		vpe_config.data_pool[0].mode = HD_VIDEOPROC_POOL_ENABLE;
		vpe_config.data_pool[0].ddr_id = 0;
		vpe_config.data_pool[0].counts = HD_VIDEOPROC_SET_COUNT(4, 0);

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

HD_RESULT set_dec_param(VIDEO_PLAYBACK *pb, int div)
{
	int i;
	HD_RESULT ret = HD_OK;
	HD_VIDEODEC_PATH_CONFIG dec_config;

	for (i = 0; i < MAX_BITSTREAM_NUM; i++) {
		memset(&dec_config, 0x0, sizeof(HD_VIDEODEC_PATH_CONFIG));
		/* Set videodec parameters */
		dec_config.max_mem.codec_type = (pb->is_h265 == 1) ? HD_CODEC_TYPE_H265 : HD_CODEC_TYPE_H264;
		dec_config.max_mem.dim.w = MAINSTREAM_WIDTH;
		dec_config.max_mem.dim.h = MAINSTREAM_HEIGHT;
		dec_config.max_mem.frame_rate = PB_FRAME_RATE;
		dec_config.max_mem.bs_counts = 7;
		dec_config.max_mem.max_ref_num = 1;

		/* Set videodec out pool */
		dec_config.data_pool[0].mode = HD_VIDEODEC_POOL_ENABLE;
		dec_config.data_pool[0].ddr_id = 0;
		dec_config.data_pool[0].counts = HD_VIDEODEC_SET_COUNT(4, 0);
		dec_config.data_pool[0].max_counts = HD_VIDEODEC_SET_COUNT(3, 0); //minus dec reference 1

		dec_config.data_pool[1].mode = HD_VIDEODEC_POOL_ENABLE;
		dec_config.data_pool[1].ddr_id = 0;
		dec_config.data_pool[1].counts = HD_VIDEODEC_SET_COUNT(4, 0);
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
	for (i = 0; i < MAX_BITSTREAM_NUM; i++) {
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

HD_RESULT pb_start(VIDEO_PLAYBACK *pb)
{
	int i;
	UINT32 ret = HD_OK;

	if (pb->is_playback == 0) {
		for (i = 0; i < MAX_BITSTREAM_NUM; i++) {
			if ((ret = hd_videodec_bind(HD_VIDEODEC_OUT(0, i), HD_VIDEOPROC_IN(i, 0))) != HD_OK) {
				printf("Error hd_videodec_unbind %lu\n", ret);
				return ret;
			}
			if ((ret = hd_videoproc_bind(HD_VIDEOPROC_OUT(i, 0), HD_VIDEOOUT_IN(0, i))) != HD_OK) {
				printf("Error hd_videoproc_unbind %lu\n", ret);
				return ret;
			}
		}

		if ((ret = hd_videodec_start_list(pb->pb_dec_list, MAX_BITSTREAM_NUM)) != HD_OK) {
			printf("Error hd_videodec_start_list pb_dec_list %lu\n", ret);
			ret = HD_ERR_NG;
			goto exit_pb_start;
		}
		if ((ret = hd_videoproc_start_list(pb->pb_vpe_list, MAX_BITSTREAM_NUM)) != HD_OK) {
			printf("Error hd_videodec_start_list pb_vpe_list %lu\n", ret);
			ret = HD_ERR_NG;
			goto exit_pb_start;
		}
		if ((ret = hd_videoout_start_list(pb->pb_lcd_list, MAX_BITSTREAM_NUM)) != HD_OK) {
			printf("Error hd_videodec_start_list pb_lcd_list %lu\n", ret);
			ret = HD_ERR_NG;
			goto exit_pb_start;
		}
	} else { /* for restart playback, only start the videoout list */
		if ((ret = hd_videoout_start_list(pb->pb_lcd_list, MAX_BITSTREAM_NUM)) != HD_OK) {
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

	if ((ret = hd_videodec_stop_list(pb->pb_dec_list, MAX_BITSTREAM_NUM)) != HD_OK) {
		printf("Error hd_videodec_stop_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_pb_stop;
	}
	if ((ret = hd_videoproc_stop_list(pb->pb_vpe_list, MAX_BITSTREAM_NUM)) != HD_OK) {
		printf("Error hd_videoproc_stop_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_pb_stop;
	}
	if ((ret = hd_videoout_stop_list(pb->pb_lcd_list, MAX_BITSTREAM_NUM)) != HD_OK) {
		printf("Error hd_videoout_stop_list %d\n", ret);
		ret = HD_ERR_NG;
		goto exit_pb_stop;
	}

	for (i = 0; i < MAX_BITSTREAM_NUM; i++) {
		if ((ret = hd_videodec_unbind(HD_VIDEODEC_OUT(0, i))) != HD_OK) {
			printf("Error hd_videodec_unbind %d\n", ret);
			break;
		}
		if ((ret = hd_videoproc_unbind(HD_VIDEOPROC_OUT(i, 0))) != HD_OK) {
			printf("Error hd_videoproc_unbind %d\n", ret);
			break;
		}
	}
exit_pb_stop:
	pthread_join(pb->playback_thread_id, NULL);
	return ret;
}

HD_RESULT init_module(void)
{
	UINT32 ret = HD_OK;
	if ((ret = hd_common_init(1)) != HD_OK) {
		printf("common init fail\n");
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

HD_RESULT open_module(VIDEO_PLAYBACK *pb)
{
	int i;
	UINT32 ret = HD_OK;

	for (i = 0; i < MAX_BITSTREAM_NUM; i++) {
		/* videodec: playback */
		if ((ret = hd_videodec_open(HD_VIDEODEC_IN(0, i), HD_VIDEODEC_OUT(0, i), &pb->pb_dec_list[i])) != HD_OK) {
			printf("Error hd_videodec_open %lu\n", ret);
			goto exit_open_module;
		}

		/* videoprocess: playback */
		if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(i, 0), HD_VIDEOPROC_OUT(i, 0),
				&pb->pb_vpe_list[i])) != HD_OK) {
			printf("Error hd_videoproc_open %lu\n", ret);
			goto exit_open_module;
		}
		/* videoput: playback */
	    if ((ret = hd_videoout_open(HD_VIDEOOUT_IN(0, i), HD_VIDEOOUT_OUT(0, 0), &pb->pb_lcd_list[i])) != HD_OK) {
			printf("Error hd_videoout_open %lu\n", ret);
			goto exit_open_module;
		}
	}
exit_open_module:
	return ret;
}

HD_RESULT close_module(VIDEO_PLAYBACK *pb)
{
	int i;
	HD_RESULT ret = HD_OK;

	for (i = 0; i < MAX_BITSTREAM_NUM; i++) {
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
	return ret;
}

HD_RESULT exit_module(void)
{
	int ret = HD_OK;

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
	int ret = 0, length = 0, ch = 0;
	FILE *bs_fd, *len_fd;
	char *playback_buffer;
	unsigned int frame_period;
	unsigned int channel_time;
	HD_VIDEODEC_SEND_LIST video_bs[MAX_BITSTREAM_NUM];
	unsigned int perf_start_ms = 0, frame_counts = 0, now_time_ms = 0;
	struct timeval time_start;
	unsigned int sleep_ms = 0;

	memset(video_bs, 0, sizeof(video_bs));
	frame_period = 1000 / pb->fps;   //frame rate 30fps
	if ((bs_fd = fopen(pb->pattern_bs_file, "rb+")) == NULL) {
		printf("[ERROR] CH%d Open %s failed!!\n", ch, pb->pattern_bs_file);
		exit(1);
	}
	printf("Playback %s frame_period %dms\n", pb->pattern_bs_file, frame_period);
	if ((len_fd = fopen(pb->pattern_len_file, "rb+")) == NULL) {
		printf("[ERROR] CH%d Open %s failed!!\n", ch, pb->pattern_len_file);
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
		if (fscanf(len_fd, "%d\n", &length) == EOF) {
			fseek(bs_fd, 0, SEEK_SET);
			fseek(len_fd, 0, SEEK_SET);
			fscanf(len_fd, "%d\n", &length);
		}
		if (length == 0) {
			break;
		}
		fread(playback_buffer, 1, length, bs_fd);

		for (ch = 0; ch < MAX_BITSTREAM_NUM; ch++) {
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
		while ((pb->is_playback) && ((ret = hd_videodec_send_list(video_bs, MAX_BITSTREAM_NUM, 500)) < 0)) {
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

void show_menu(void)
{
	printf("\nUsage:nvr_perf [is_h265] [fps]\n");
	printf("===============================\n");	
	printf(" (P)Playback\n");
	printf(" (D)Debug Menu\n");
	printf(" (Q)Exit\n");
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	int key, is_exit = 0, pb_div = 0;
	int run_inputs = MAX_BITSTREAM_NUM;
	VIDEO_PLAYBACK pb;

	/* init default value */
	pb.is_h265 = 0; 
	pb.fps = PB_FRAME_RATE;

	if (argc > 2) {
		pb.is_h265 = atoi(argv[1]);
		pb.fps = atoi(argv[2]);
	}

	if (pb.is_h265) {
		strcpy(pb.pattern_bs_file, PATTERN_H265_BS_FILE);
		strcpy(pb.pattern_len_file, PATTERN_H265_LEN_FILE);
	} else {
		strcpy(pb.pattern_bs_file, PATTERN_H264_BS_FILE);
		strcpy(pb.pattern_len_file, PATTERN_H264_LEN_FILE);
	}

	printf("nvr_perf %s Decode(%s) fps(%d)\n", NVR_PERF_VERSION, (pb.is_h265 == 1)? "H265" : "AUTO", pb.fps);

	pb.is_playback = 0;	
	if (init_module() != HD_OK) {
		printf("Error init_module!\n");
		goto exit_main;
	}
	if (open_module(&pb) != HD_OK) {
		printf("Error open_module!\n");
		goto exit_main;
	}

	if (get_pb_lcd_caps(&pb) != HD_OK) {
		printf("Error get_pb_lcd_caps!\n");
		goto exit_main;
	}

	show_menu();
	while (is_exit == 0) {
		key = getchar();
		switch (key) {
		case 'P': case 'p':
			if (pb_div > 1) {
				pb_div--;
			} else if (run_inputs == 8) { //8ch
				pb_div = 3;
			} else if (run_inputs == 4) { //4ch
				pb_div = 2;
			}
			printf("Start %sPlayback (div%d)...\n", (pb.is_h265 == 1)? "H265 " : "", pb_div * pb_div);
			set_pb_vpe_param(&pb, pb_div);
			set_dec_param(&pb, pb_div);
			set_pb_lcd_param(&pb, pb_div);
			pb_start(&pb);
			break;
		case 'd': case 'D':
			hd_debug_run_menu();
			break;
		case 'Q': case 'q':
			pb_stop(&pb);
			is_exit = 1;
			break;
		default:
			show_menu();
			break;
		}
	}

	pb_stop(&pb);
	if (close_module(&pb) != HD_OK) {
		printf("Error close_module!\n");
		goto exit_main;
	}
	if (exit_module() != HD_OK) {
		printf("Error exit_module!\n");
	}
exit_main:
	return 0;
}

