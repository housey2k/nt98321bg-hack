/**
     @file module_init.c
     @brief :get product spec from dts file
     Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <comm/nvtmem_if.h>
#include "hdal.h"
#include "vendor_ad.h"
#include "vendor_videoout.h"
#include "vendor_ad_tp28xx.h"
#include "vendor_ad_nvp61xx.h"
#include "libfdt.h"
#include "vendor_common.h"
#define MODULE_VERSION  "v2.d"
//char DTB_PATH[] = "/usr/module_init_cfg.dtb";
char DTB_PATH[] = "./module_init_cfg.dtb";
//FOR FPGA
int is_init_videoout = 1;
int is_init_videocap = 2;
int is_init_audio = 1;
int is_show_logo = 0;
//

//#define DT_MEME_POOL_NUM       128
#define DT_NODE_SZ             64
#define COMMON_MEM_SHARED_POOL_NUM           4
#define ALIGN16_UP(x)       ALIGN_CEIL(x, 16)      /* for 16 bytes alignment */
#define ALIGN256_UP(x)      ALIGN_CEIL(x, 256)     /* for 256 bytes alignment */
#define ALIGN4096_UP(x)     ALIGN_CEIL(x, 4096)    /* for hw dma 4K bytes alignment */
#define SET_SHAREPOOL_VAL(id,idx)	((id & 0xFF) << (idx << 3))
int dump_dts_info = 0;
extern HD_RESULT show_logo(unsigned int buf_paddr, unsigned int buf_size, int fb_pxlfmt, int width, int height);
typedef struct _COMMON_MEM_POOL_INFO {
	HD_COMMON_MEM_DDR_ID     ddr_id;                                     ///< ddr id
	HD_COMMON_MEM_POOL_TYPE  type;                                       ///< main pool type
	UINT32                   blk_size;                                   ///< the block size of this pool
	UINT32                   blk_cnt;                                    ///< the block count of this pool
	UINT32                   start_addr;                                 ///< the starting address of this pool
	HD_COMMON_MEM_POOL_TYPE  shared_pool[HD_COMMON_MEM_SHARED_POOL_NUM]; ///< indicate which pool type is shared with main pool
} COMMON_MEM_POOL_INFO;
#define __to_str(x)  #x

typedef struct _VIDEOOUT_INFO {
	HD_VIDEOOUT_DEVNVR_CONFIG dev_cfg;
	HD_FB_FMT fb_cfg[3];
	HD_FB_ENABLE fb_state[3];
} VIDEOOUT_INFO;

typedef struct {
	VIDEOOUT_INFO videoout_0;
	VIDEOOUT_INFO videoout_1;
	HD_AUDIOCAP_DRV_CONFIG audio_drv_cfg;
	HD_AUDIOOUT_DRV_CONFIG audioout_drv_conf;
	HD_VIDEOCAP_HOST vcap_host;
	COMMON_MEM_POOL_INFO mem_pool_info[HD_COMMON_MEM_MAX_POOL_NUM];
} DT_HDAL_SPEC;

DT_HDAL_SPEC dt_hdal_spec = {0};
#define AD_DEV_NODE_NAME  "/dev/tp2823dev" // AD device node name
#define AD_DEV_NODE_NAME_NVP  "/dev/nc_vdec" // AD device node name

typedef struct _MODULE_INIT_INFO {
	HD_PATH_ID video_0_ctrl;
	HD_PATH_ID video_1_ctrl;
	HD_PATH_ID video_2_ctrl;
	HD_PATH_ID audio_cap_ctrl;
	HD_PATH_ID audio_out_ctrl;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg;
} MODULE_INIT_INFO;

typedef struct _MODULE_TP28XX_INFO {
	VENDOR_AD_TP28XX_VIDEO_NORM g_ch_norm[VENDOR_AD_PLAT_CHIP_VI_MAX][VENDOR_AD_PLAT_VCAP_VI_MAX];
	INT g_ch_loss[VENDOR_AD_PLAT_CHIP_VI_MAX][VENDOR_AD_PLAT_VCAP_VI_MAX];
} MODULE_TP28XX_INFO;

typedef struct _MODULE_NVP61XX_INFO {
	VENDOR_AD_TP28XX_VIDEO_NORM g_ch_norm[VENDOR_AD_PLAT_CHIP_VI_MAX][VENDOR_AD_PLAT_VCAP_VI_MAX];
	INT g_ch_loss[VENDOR_AD_PLAT_CHIP_VI_MAX][VENDOR_AD_PLAT_VCAP_VI_MAX];
} MODULE_NVP61XX_INFO;

HD_RESULT videocap_module_init_nvp(MODULE_NVP61XX_INFO *p_init_info, CHAR *ad_dev_name)
{
	HD_RESULT ret = HD_OK;
	INT i, j;
	INT notify_polling = 1;
	HD_VIDEOCAP_HOST_ID vcap_host_id;
	HD_VIDEOCAP_HOST vcap_host;
	HD_VIDEOCAP_VI vcap_vi;
	HD_VIDEOCAP_VI_ID vcap_vi_id;
	HD_VIDEOCAP_VI_CH_PARAM ch_param = {0};
	HD_VIDEOCAP_VI_CH_NORM ch_norm = {0};
	VENDOR_AD_NVP61XX_DEVICE_INFO dev_info;
	VENDOR_AD_NVP61XX_VIDEO_NORM video_norm;
	VENDOR_AD_NVP61XX_VIDEO_LOSS ch_loss;

	ret = vendor_ad_init(ad_dev_name);
	if (ret != HD_OK) {
		goto exit;
	}

	/* get nc device info */
	ret = vendor_ad_get(VENDOR_AD_PARAM_NVP61XX_DEVICE_INFO, &dev_info);
	if (ret != HD_OK) {
		goto exit;
	}

	/* deregister all vcap vi to force clear previous setting */
	for (i = 0; i < HD_VIDEOCAP_VI_MAX; i++) {
		vcap_vi_id.chip    = VENDOR_AD_PLAT_VI_TO_CHIP_ID(i);
		vcap_vi_id.vcap    = VENDOR_AD_PLAT_VI_TO_CHIP_VCAP_ID(i);
		vcap_vi_id.vi      = VENDOR_AD_PLAT_VI_TO_VCAP_VI_ID(i);
		ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_DEREGISTER_VI, &vcap_vi_id);
		if (ret != HD_OK) {
			goto exit;
		}
	}

	/* uninit vcap host to force clear previous setting */
	vcap_host_id.host    = 0;
	ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_UNINIT_HOST, &vcap_host_id);
	if (ret != HD_OK) {
		goto exit;
	}

	/* vcap host init, to specify vcap system vi usage and prepare requirement memory */
	memset(&vcap_host, 0, sizeof(vcap_host));
	vcap_host.host            = 0;//only support 0
	vcap_host.md.enable       = dt_hdal_spec.vcap_host.md.enable;
	vcap_host.md.mb_x_num_max = dt_hdal_spec.vcap_host.md.mb_x_num_max;
	vcap_host.md.mb_y_num_max = dt_hdal_spec.vcap_host.md.mb_y_num_max;
	vcap_host.md.buf_src      = dt_hdal_spec.vcap_host.md.buf_src;
	for (i = 0; i < dev_info.dev_num; i++) {
		for (j = 0; j < VENDOR_AD_NVP61XX_VOUT_MAX; j++) {
			if (!dev_info.dev[i].vout[j].xcap || !dev_info.dev[i].vout[j].vi) { ///< VOUT# Connect to X_CAP# and Grab VI#
				continue;
			}
			if (vcap_host.nr_of_vi >= HD_VIDEOCAP_VI_MAX) {
				printf("vcap_host.nr_of_vi(%u) >= HD_VIDEOCAP_VI_MAX(%u)\n", vcap_host.nr_of_vi, HD_VIDEOCAP_VI_MAX);
				continue;
			}
			vcap_host.vi[vcap_host.nr_of_vi].chip = VENDOR_AD_PLAT_VI_TO_CHIP_ID(dev_info.dev[i].vout[j].vi - 1);
			vcap_host.vi[vcap_host.nr_of_vi].vcap = VENDOR_AD_PLAT_VI_TO_CHIP_VCAP_ID(dev_info.dev[i].vout[j].vi - 1);
			vcap_host.vi[vcap_host.nr_of_vi].vi   = VENDOR_AD_PLAT_VI_TO_VCAP_VI_ID(dev_info.dev[i].vout[j].vi - 1);
			switch (dev_info.dev[i].vout_mode) {
			case VENDOR_AD_NVP61XX_VOUT_MODE_2CH_DUAL_EDGE:
			case VENDOR_AD_NVP61XX_VOUT_MODE_2CH_MUX:
			case VENDOR_AD_NVP61XX_VOUT_MODE_2CH_DUAL_EDGE_297MHZ:
			case VENDOR_AD_NVP61XX_VOUT_MODE_2CH_MUX_297MHZ:
				vcap_host.vi[vcap_host.nr_of_vi].mode = HD_VIDEOCAP_VI_MODE_2CH;
				break;
			case VENDOR_AD_NVP61XX_VOUT_MODE_4CH_MUX:
			case VENDOR_AD_NVP61XX_VOUT_MODE_4CH_MUX_297MHZ:
				vcap_host.vi[vcap_host.nr_of_vi].mode = HD_VIDEOCAP_VI_MODE_4CH;
				break;
			case VENDOR_AD_NVP61XX_VOUT_MODE_4CH_DUAL_EDGE_297MHZ:
			case VENDOR_AD_NVP61XX_VOUT_MODE_4CH_DUAL_EDGE:
				vcap_host.vi[vcap_host.nr_of_vi].mode = HD_VIDEOCAP_VI_MODE_4CH_2P;
				break;
			case VENDOR_AD_NVP61XX_VOUT_MODE_1CH_BYPASS:
			default:
				vcap_host.vi[vcap_host.nr_of_vi].mode = HD_VIDEOCAP_VI_MODE_1CH;
				break;
			}

			vcap_host.nr_of_vi++;
			if (vcap_host.nr_of_vi > HD_VIDEOCAP_VI_MAX) {
				printf("vcap_host.nr_of_vi(%u) >= HD_VIDEOCAP_VI_MAX(%u)\n", vcap_host.nr_of_vi, HD_VIDEOCAP_VI_MAX);
				continue;
			}
		}
	}
	if (vcap_host.nr_of_vi == 0) {
		goto exit;
	}

	ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_INIT_HOST, &vcap_host);
	if (ret != HD_OK) {
		goto exit;
	}

	/* VCAP VI Register */
	for (i = 0; i < dev_info.dev_num; i++) {
		for (j = 0; j < VENDOR_AD_NVP61XX_VOUT_MAX; j++) {
			if (!dev_info.dev[i].vout[j].xcap || !dev_info.dev[i].vout[j].vi) { ///< VOUT# Connect to X_CAP# and Grab VI#
				continue;
			}

			memset(&vcap_vi, 0, sizeof(vcap_vi));

			vcap_vi.chip    = VENDOR_AD_PLAT_VI_TO_CHIP_ID(dev_info.dev[i].vout[j].vi - 1);
			vcap_vi.vcap    = VENDOR_AD_PLAT_VI_TO_CHIP_VCAP_ID(dev_info.dev[i].vout[j].vi - 1);
			vcap_vi.vi      = VENDOR_AD_PLAT_VI_TO_VCAP_VI_ID(dev_info.dev[i].vout[j].vi - 1);

			vcap_vi.global.src        = dev_info.dev[i].vout[j].xcap - 1;
			vcap_vi.global.format     = HD_VIDEOCAP_VI_FMT_BT656;
			vcap_vi.global.id_extract = HD_VIDEOCAP_VI_CHID_EAV_SAV;
			switch (dev_info.dev[i].vout_mode) {
			case VENDOR_AD_NVP61XX_VOUT_MODE_2CH_DUAL_EDGE:
			case VENDOR_AD_NVP61XX_VOUT_MODE_2CH_DUAL_EDGE_297MHZ:
				vcap_vi.global.tdm        = HD_VIDEOCAP_VI_TDM_2CH_DUALEDGE;
				vcap_vi.global.latch_edge = HD_VIDEOCAP_VI_LATCH_EDGE_DUAL;
				break;
			case VENDOR_AD_NVP61XX_VOUT_MODE_2CH_MUX:
			case VENDOR_AD_NVP61XX_VOUT_MODE_2CH_MUX_297MHZ:
				vcap_vi.global.tdm        = HD_VIDEOCAP_VI_TDM_2CH_MUX;
				vcap_vi.global.latch_edge = HD_VIDEOCAP_VI_LATCH_EDGE_SINGLE;
				break;
			case VENDOR_AD_NVP61XX_VOUT_MODE_4CH_MUX:
			case VENDOR_AD_NVP61XX_VOUT_MODE_4CH_MUX_297MHZ:
				vcap_vi.global.tdm        = HD_VIDEOCAP_VI_TDM_4CH_MUX;
				vcap_vi.global.latch_edge = HD_VIDEOCAP_VI_LATCH_EDGE_SINGLE;
				break;
                    break;
			case VENDOR_AD_NVP61XX_VOUT_MODE_4CH_DUAL_EDGE:
			case VENDOR_AD_NVP61XX_VOUT_MODE_4CH_DUAL_EDGE_297MHZ:
				vcap_vi.global.tdm        = HD_VIDEOCAP_VI_TDM_4CH_2P_MUX;
				vcap_vi.global.latch_edge = HD_VIDEOCAP_VI_LATCH_EDGE_DUAL;
				break;
			case VENDOR_AD_NVP61XX_VOUT_MODE_1CH_BYPASS:
			default:
				vcap_vi.global.tdm        = HD_VIDEOCAP_VI_TDM_1CH_BYPASS;
				vcap_vi.global.latch_edge = HD_VIDEOCAP_VI_LATCH_EDGE_SINGLE;
				break;
			}
			vcap_vi.vport[0].clk_inv   = dev_info.dev[i].vout[j].clk_inv;
			vcap_vi.vport[0].clk_dly   = dev_info.dev[i].vout[j].clk_dly;
			vcap_vi.vport[0].clk_pdly  = dev_info.dev[i].vout[j].clk_pdly;
			vcap_vi.vport[0].clk_pin   = dev_info.dev[i].vout[j].clk_pin;
			vcap_vi.vport[0].data_swap = dev_info.dev[i].vout[j].data_swap;   ///< VI-P0(rising)
			if (vcap_vi.global.latch_edge == HD_VIDEOCAP_VI_LATCH_EDGE_DUAL) {
				vcap_vi.vport[1].data_swap = vcap_vi.vport[0].data_swap;    ///< VI-P1(falling)
			}
			ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_REGISTER_VI, &vcap_vi);
			if (ret != HD_OK) {
				goto exit;
			}
		}
	}

	/* VCH ID */
	for (i = 0; i < dev_info.dev_num; i++) {
		for (j = 0; j < VENDOR_AD_NVP61XX_CHANNELS_PER_CHIP; j++) {
			if (!dev_info.dev[i].vin[j].active) {
				continue;
			}

			memset(&ch_param, 0, sizeof(ch_param));

			ch_param.chip    = dev_info.dev[i].vin[j].chip;
			ch_param.vcap    = dev_info.dev[i].vin[j].vcap;
			ch_param.vi      = dev_info.dev[i].vin[j].vi;
			ch_param.ch      = dev_info.dev[i].vin[j].ch;
			ch_param.value   = dev_info.dev[i].vin[j].vch_id;
			ch_param.pid     = HD_VIDEOCAP_VI_CH_PARAM_VCH_ID;
			ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_VI_CH_PARAM, &ch_param);
			if (ret != HD_OK) {
				goto exit;
			}
		}
	}

	/* Polling channel video norm to notify vcap channel norm switch */
	do {
		for (i = 0; i < dev_info.dev_num; i++) {
			for (j = 0; j < VENDOR_AD_NVP61XX_CHANNELS_PER_CHIP; j++) {
				if (!dev_info.dev[i].vin[j].active) {
					continue;
				}

				/* get video norm */
				memset(&video_norm, 0, sizeof(video_norm));
				video_norm.dev_id = i;
				video_norm.vin_id = j;
				ret = vendor_ad_get(VENDOR_AD_PARAM_NVP61XX_VIDEO_NORM, &video_norm);
				if (ret != HD_OK) {
					goto chk_loss;
				}

				/* check video norm */
				if (memcmp(&video_norm, &p_init_info->g_ch_norm[i][j], sizeof(video_norm)) == 0) {
					goto chk_loss;
				}

				ch_norm.chip       = dev_info.dev[i].vin[j].chip;
				ch_norm.vcap       = dev_info.dev[i].vin[j].vcap;
				ch_norm.vi         = dev_info.dev[i].vin[j].vi;
				ch_norm.ch         = dev_info.dev[i].vin[j].ch;
				ch_norm.org_width  = video_norm.src_width;
				ch_norm.org_height = video_norm.src_height;
				ch_norm.fps        = video_norm.src_fps;
				ch_norm.prog       = video_norm.src_prog;
				ch_norm.cap_width  = video_norm.out_width;
				ch_norm.cap_height = video_norm.out_height;
				ch_norm.format     = video_norm.out_fmt;
				ch_norm.data_rate  = video_norm.out_data_rate;
				ch_norm.data_latch = video_norm.out_data_latch;
				ch_norm.horiz_dup  = video_norm.out_horiz_dup;
				ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_VI_CH_NORM, &ch_norm);
				if (ret != HD_OK) {
					goto chk_loss;
				}

				memcpy(&p_init_info->g_ch_norm[i][j], &video_norm, sizeof(video_norm));

				fprintf(stdout, "nvpxx#%d-vin#%d norm switch to %lux%lu%s@%lu\n",
						i, j,
						video_norm.src_width,
						video_norm.src_height,
						video_norm.src_prog ? "P" : "I",
						video_norm.src_fps);

chk_loss:
				/* get video loss */
				ch_loss.chip = i;
				ch_loss.ch   = j;
				ret = vendor_ad_get(VENDOR_AD_PARAM_NVP61XX_VIDEO_LOSS, &ch_loss);
				if (ret != HD_OK) {
					continue;
				}

				/* check video loss */
				if (ch_loss.is_lost == p_init_info->g_ch_loss[i][j]) {
					continue;
				}

				ch_param.chip    = dev_info.dev[i].vin[j].chip;
				ch_param.vcap    = dev_info.dev[i].vin[j].vcap;
				ch_param.vi      = dev_info.dev[i].vin[j].vi;
				ch_param.ch      = dev_info.dev[i].vin[j].ch;
				ch_param.value   = ch_loss.is_lost;
				ch_param.pid     = HD_VIDEOCAP_VI_CH_PARAM_VLOS;
				ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_VI_CH_PARAM, &ch_param);
				if (ret != HD_OK) {
					goto chk_loss;
				}

				p_init_info->g_ch_loss[i][j] = ch_loss.is_lost;

				fprintf(stdout, "nvp61xx#%d-vin#%d video %s\n",
						i, j,
						ch_loss.is_lost ? "loss" : "present");
			}
		}
		usleep(500000);
	} while (notify_polling);

exit:
	ret = vendor_ad_uninit();
	return ret;
}

HD_RESULT videocap_module_init_tp(MODULE_TP28XX_INFO *p_init_info, CHAR *ad_dev_name)
{
	HD_RESULT ret = HD_OK;
	INT i, j;
	INT notify_polling = 1;
	HD_VIDEOCAP_HOST_ID vcap_host_id;
	HD_VIDEOCAP_HOST vcap_host;
	HD_VIDEOCAP_VI vcap_vi;
	HD_VIDEOCAP_VI_ID vcap_vi_id;
	HD_VIDEOCAP_VI_CH_PARAM ch_param = {0};
	HD_VIDEOCAP_VI_CH_NORM ch_norm= {0};
	VENDOR_AD_TP28XX_DEVICE_INFO dev_info;
	VENDOR_AD_TP28XX_VIDEO_NORM video_norm;
	VENDOR_AD_TP28XX_VIDEO_LOSS ch_loss;

	ret = vendor_ad_init(ad_dev_name);
	if (ret != HD_OK) {
		goto exit;
	}

	/* get tp28xx device info */
	ret = vendor_ad_get(VENDOR_AD_PARAM_TP28XX_DEVICE_INFO, &dev_info);
	if (ret != HD_OK) {
		goto exit;
	}

	/* deregister all vcap vi to force clear previous setting */
	for (i = 0; i < HD_VIDEOCAP_VI_MAX; i++) {
		vcap_vi_id.chip    = VENDOR_AD_PLAT_VI_TO_CHIP_ID(i);
		vcap_vi_id.vcap    = VENDOR_AD_PLAT_VI_TO_CHIP_VCAP_ID(i);
		vcap_vi_id.vi      = VENDOR_AD_PLAT_VI_TO_VCAP_VI_ID(i);
		ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_DEREGISTER_VI, &vcap_vi_id);
		if (ret != HD_OK) {
			goto exit;
		}
	}

	/* uninit vcap host to force clear previous setting */
	vcap_host_id.host    = 0;
	ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_UNINIT_HOST, &vcap_host_id);
	if (ret != HD_OK) {
		goto exit;
	}

	/* vcap host init, to specify vcap system vi usage and prepare requirement memory */
	memset(&vcap_host, 0, sizeof(vcap_host));
	vcap_host.host            = 0;//only support 0
	vcap_host.md.enable       = dt_hdal_spec.vcap_host.md.enable;
	vcap_host.md.mb_x_num_max = dt_hdal_spec.vcap_host.md.mb_x_num_max;
	vcap_host.md.mb_y_num_max = dt_hdal_spec.vcap_host.md.mb_y_num_max;
	vcap_host.md.buf_src      = dt_hdal_spec.vcap_host.md.buf_src;
	for (i = 0; i < dev_info.dev_num; i++) {
		for (j = 0; j < VENDOR_AD_TP28XX_VOUT_MAX; j++) {
			if (!dev_info.dev[i].vout[j].xcap || !dev_info.dev[i].vout[j].vi) { ///< VOUT# Connect to X_CAP# and Grab VI#
				continue;
			}
			if (vcap_host.nr_of_vi >= HD_VIDEOCAP_VI_MAX) {
				printf("vcap_host.nr_of_vi(%u) >= HD_VIDEOCAP_VI_MAX(%u)\n", vcap_host.nr_of_vi, HD_VIDEOCAP_VI_MAX);
				continue;
			}
			vcap_host.vi[vcap_host.nr_of_vi].chip = VENDOR_AD_PLAT_VI_TO_CHIP_ID(dev_info.dev[i].vout[j].vi - 1);
			vcap_host.vi[vcap_host.nr_of_vi].vcap = VENDOR_AD_PLAT_VI_TO_CHIP_VCAP_ID(dev_info.dev[i].vout[j].vi - 1);
			vcap_host.vi[vcap_host.nr_of_vi].vi   = VENDOR_AD_PLAT_VI_TO_VCAP_VI_ID(dev_info.dev[i].vout[j].vi - 1);
			switch (dev_info.dev[i].vout_mode) {
			case VENDOR_AD_TP28XX_VOUT_MODE_SDR_2CH_DUAL_EDGE:
			case VENDOR_AD_TP28XX_VOUT_MODE_SDR_2CH_MUX:
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_2CH_DUAL_EDGE:
				vcap_host.vi[vcap_host.nr_of_vi].mode = HD_VIDEOCAP_VI_MODE_2CH;
				break;
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_4CH_MUX_DUAL_EDGE:
				vcap_host.vi[vcap_host.nr_of_vi].mode = HD_VIDEOCAP_VI_MODE_4CH_2P;
				break;
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_4CH_MUX_DUAL_EDGE_2VI:
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_2CH_DUAL_EDGE_2VI:
				vcap_host.vi[vcap_host.nr_of_vi].mode = (dev_info.dev[i].vout_mode == VENDOR_AD_TP28XX_VOUT_MODE_DDR_4CH_MUX_DUAL_EDGE_2VI) ? HD_VIDEOCAP_VI_MODE_2CH : HD_VIDEOCAP_VI_MODE_1CH;
				vcap_host.nr_of_vi++;
				if (vcap_host.nr_of_vi >= HD_VIDEOCAP_VI_MAX) {
					printf("vcap_host.nr_of_vi(%u) >= HD_VIDEOCAP_VI_MAX(%u)\n", vcap_host.nr_of_vi, HD_VIDEOCAP_VI_MAX);
					continue;
				}
				vcap_host.vi[vcap_host.nr_of_vi].chip = vcap_host.vi[vcap_host.nr_of_vi - 1].chip;
				vcap_host.vi[vcap_host.nr_of_vi].vcap = vcap_host.vi[vcap_host.nr_of_vi - 1].vcap;
				vcap_host.vi[vcap_host.nr_of_vi].vi   = vcap_host.vi[vcap_host.nr_of_vi - 1].vi + (VENDOR_AD_PLAT_VCAP_VI_MAX>>1);
				vcap_host.vi[vcap_host.nr_of_vi].mode = vcap_host.vi[vcap_host.nr_of_vi - 1].mode;
				break;
			case VENDOR_AD_TP28XX_VOUT_MODE_SDR_1CH_BYPASS:
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_1CH_BYPASS:
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_1CH_16BIT_BYPASS:
			default:
				vcap_host.vi[vcap_host.nr_of_vi].mode = HD_VIDEOCAP_VI_MODE_1CH;
				break;
			}

			vcap_host.nr_of_vi++;
			if (vcap_host.nr_of_vi > HD_VIDEOCAP_VI_MAX) {
				printf("vcap_host.nr_of_vi(%u) >= HD_VIDEOCAP_VI_MAX(%u)\n", vcap_host.nr_of_vi, HD_VIDEOCAP_VI_MAX);
				continue;
			}
		}
	}
	if (vcap_host.nr_of_vi == 0) {
		goto exit;
	}

	ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_INIT_HOST, &vcap_host);
	if (ret != HD_OK) {
		goto exit;
	}

	/* VCAP VI Register */
	for (i = 0; i < dev_info.dev_num; i++) {
		for (j = 0; j < VENDOR_AD_TP28XX_CHANNELS_PER_CHIP; j++) {
			if (!dev_info.dev[i].vout[j].xcap || !dev_info.dev[i].vout[j].vi) { ///< VOUT# Connect to X_CAP# and Grab VI#
				continue;
			}

			memset(&vcap_vi, 0, sizeof(vcap_vi));

			vcap_vi.chip    = VENDOR_AD_PLAT_VI_TO_CHIP_ID(dev_info.dev[i].vout[j].vi - 1);
			vcap_vi.vcap    = VENDOR_AD_PLAT_VI_TO_CHIP_VCAP_ID(dev_info.dev[i].vout[j].vi - 1);
			vcap_vi.vi      = VENDOR_AD_PLAT_VI_TO_VCAP_VI_ID(dev_info.dev[i].vout[j].vi - 1);

			vcap_vi.global.src        = dev_info.dev[i].vout[j].xcap - 1;
			vcap_vi.global.format     = HD_VIDEOCAP_VI_FMT_BT656;
			vcap_vi.global.id_extract = HD_VIDEOCAP_VI_CHID_EAV_SAV;
			switch (dev_info.dev[i].vout_mode) {
			case VENDOR_AD_TP28XX_VOUT_MODE_SDR_2CH_DUAL_EDGE:
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_2CH_DUAL_EDGE:
				vcap_vi.global.tdm        = HD_VIDEOCAP_VI_TDM_2CH_DUALEDGE;
				vcap_vi.global.latch_edge = HD_VIDEOCAP_VI_LATCH_EDGE_DUAL;
				break;
			case VENDOR_AD_TP28XX_VOUT_MODE_SDR_2CH_MUX:
				vcap_vi.global.tdm        = HD_VIDEOCAP_VI_TDM_2CH_MUX;
				vcap_vi.global.latch_edge = HD_VIDEOCAP_VI_LATCH_EDGE_SINGLE;
				break;
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_4CH_MUX_DUAL_EDGE:
				vcap_vi.global.tdm        = HD_VIDEOCAP_VI_TDM_4CH_2P_MUX;
				vcap_vi.global.latch_edge = HD_VIDEOCAP_VI_LATCH_EDGE_DUAL;
				break;
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_4CH_MUX_DUAL_EDGE_2VI:
				vcap_vi.global.tdm        = HD_VIDEOCAP_VI_TDM_2CH_MUX;
				vcap_vi.global.latch_edge = HD_VIDEOCAP_VI_LATCH_EDGE_SINGLE;
				break;
			case VENDOR_AD_TP28XX_VOUT_MODE_SDR_1CH_BYPASS:
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_1CH_BYPASS:
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_1CH_16BIT_BYPASS:
			case VENDOR_AD_TP28XX_VOUT_MODE_DDR_2CH_DUAL_EDGE_2VI:
			default:
				vcap_vi.global.tdm        = HD_VIDEOCAP_VI_TDM_1CH_BYPASS;
				vcap_vi.global.latch_edge = HD_VIDEOCAP_VI_LATCH_EDGE_SINGLE;
				break;
			}
			vcap_vi.vport[0].clk_inv   = dev_info.dev[i].vout[j].clk_inv;
			vcap_vi.vport[0].clk_dly   = dev_info.dev[i].vout[j].clk_dly;
			vcap_vi.vport[0].clk_pdly  = dev_info.dev[i].vout[j].clk_pdly;
			vcap_vi.vport[0].clk_pin   = dev_info.dev[i].vout[j].clk_pin;
			vcap_vi.vport[0].data_swap = dev_info.dev[i].vout[j].data_swap;   ///< VI-P0(rising)
			if (vcap_vi.global.latch_edge == HD_VIDEOCAP_VI_LATCH_EDGE_DUAL) {
				vcap_vi.vport[1].data_swap = vcap_vi.vport[0].data_swap;    ///< VI-P1(falling)
			}
			ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_REGISTER_VI, &vcap_vi);
			if (ret != HD_OK) {
				goto exit;
			}

			/* to register internal VI for 2VI mode */
			if (dev_info.dev[i].vout_mode == VENDOR_AD_TP28XX_VOUT_MODE_DDR_4CH_MUX_DUAL_EDGE_2VI ||
				dev_info.dev[i].vout_mode == VENDOR_AD_TP28XX_VOUT_MODE_DDR_2CH_DUAL_EDGE_2VI) {
				vcap_vi.vi += (VENDOR_AD_PLAT_VCAP_VI_MAX>>1);
				vcap_vi.vport[0].clk_inv = (vcap_vi.vport[0].clk_inv) ? 0 : 1;

				ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_REGISTER_VI, &vcap_vi);
				if (ret != HD_OK) {
					printf("HD_VIDEOCAP_DRV_PARAM_REGISTER_VI 2VI fails.\n");
					goto exit;
                }
			}
		}
	}

	/* VCH ID */
	for (i = 0; i < dev_info.dev_num; i++) {
		for (j = 0; j < VENDOR_AD_TP28XX_CHANNELS_PER_CHIP; j++) {
			if (!dev_info.dev[i].vin[j].active) {
				continue;
			}

			memset(&ch_param, 0, sizeof(ch_param));

			ch_param.chip    = dev_info.dev[i].vin[j].chip;
			ch_param.vcap    = dev_info.dev[i].vin[j].vcap;
			ch_param.vi      = dev_info.dev[i].vin[j].vi;
			ch_param.ch      = dev_info.dev[i].vin[j].ch;
			ch_param.value   = dev_info.dev[i].vin[j].vch_id;
			ch_param.pid     = HD_VIDEOCAP_VI_CH_PARAM_VCH_ID;
			ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_VI_CH_PARAM, &ch_param);
			if (ret != HD_OK) {
				goto exit;
			}
		}
	}

	/* Polling channel video norm to notify vcap channel norm switch */
	do {
		for (i = 0; i < dev_info.dev_num; i++) {
			for (j = 0; j < VENDOR_AD_TP28XX_CHANNELS_PER_CHIP; j++) {
				if (!dev_info.dev[i].vin[j].active) {
					continue;
				}

				/* get video norm */
				memset(&video_norm, 0, sizeof(video_norm));
				video_norm.dev_id = i;
				video_norm.vin_id = j;
				ret = vendor_ad_get(VENDOR_AD_PARAM_TP28XX_VIDEO_NORM, &video_norm);
				if (ret != HD_OK) {
					goto chk_loss;
				}

				/* check video norm */
				if (memcmp(&video_norm, &p_init_info->g_ch_norm[i][j], sizeof(video_norm)) == 0) {
					goto chk_loss;
				}

				ch_norm.chip       = dev_info.dev[i].vin[j].chip;
				ch_norm.vcap       = dev_info.dev[i].vin[j].vcap;
				ch_norm.vi         = dev_info.dev[i].vin[j].vi;
				ch_norm.ch         = dev_info.dev[i].vin[j].ch;
				ch_norm.org_width  = video_norm.src_width;
				ch_norm.org_height = video_norm.src_height;
				ch_norm.fps        = video_norm.src_fps;
				ch_norm.prog       = video_norm.src_prog;
				ch_norm.cap_width  = video_norm.out_width;
				ch_norm.cap_height = video_norm.out_height;
				ch_norm.format     = video_norm.out_fmt;
				ch_norm.data_rate  = video_norm.out_data_rate;
				ch_norm.data_latch = video_norm.out_data_latch;
				ch_norm.horiz_dup  = video_norm.out_horiz_dup;
				ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_VI_CH_NORM, &ch_norm);
				if (ret != HD_OK) {
					goto chk_loss;
				}

				memcpy(&p_init_info->g_ch_norm[i][j], &video_norm, sizeof(video_norm));

				fprintf(stdout, "tp28xx#%d-vin#%d norm switch to %lux%lu%s@%lu\n",
						i, j,
						video_norm.src_width,
						video_norm.src_height,
						video_norm.src_prog ? "P" : "I",
						video_norm.src_fps);

chk_loss:
				/* get video loss */
				ch_loss.chip = i;
				ch_loss.ch   = j;
				ret = vendor_ad_get(VENDOR_AD_PARAM_TP28XX_VIDEO_LOSS, &ch_loss);
				if (ret != HD_OK) {
					continue;
				}

				/* check video loss */
				if (ch_loss.is_lost == p_init_info->g_ch_loss[i][j]) {
					continue;
				}

				ch_param.chip    = dev_info.dev[i].vin[j].chip;
				ch_param.vcap    = dev_info.dev[i].vin[j].vcap;
				ch_param.vi      = dev_info.dev[i].vin[j].vi;
				ch_param.ch      = dev_info.dev[i].vin[j].ch;
				ch_param.value   = ch_loss.is_lost;
				ch_param.pid     = HD_VIDEOCAP_VI_CH_PARAM_VLOS;
				ret = hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_VI_CH_PARAM, &ch_param);
				if (ret != HD_OK) {
					goto chk_loss;
				}

				p_init_info->g_ch_loss[i][j] = ch_loss.is_lost;

				fprintf(stdout, "tp28xx#%d-vin#%d video %s\n",
						i, j,
						ch_loss.is_lost ? "loss" : "present");
			}
		}
		usleep(500000);
	} while (notify_polling);

exit:
	ret = vendor_ad_uninit();
	return ret;
}

HD_RESULT audio_module_init(MODULE_INIT_INFO *p_init_info)
{
	HD_RESULT ret;

	ret = hd_common_init(1);
	if (ret != HD_OK) {
		printf("common init fail\n");
		goto ext;
	}

	if ((ret = hd_audiocap_init()) != HD_OK) {
		goto ext;
	}
	if ((ret = hd_audioout_init()) != HD_OK) {
		goto ext;
	}

	if ((ret = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &p_init_info->audio_cap_ctrl)) != HD_OK) {
		goto ext;
	}
	if ((ret = hd_audioout_open(0, HD_AUDIOOUT_0_CTRL, &p_init_info->audio_out_ctrl)) != HD_OK) {
		goto ext;
	}

	/* following is setting value */

	ret = hd_audiocap_set(p_init_info->audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, &dt_hdal_spec.audio_drv_cfg);
	if (ret != HD_OK) {
		printf("hd_audiocap_set(HD_AUDIOCAP_PARAM_DRV_CONFIG) fail\n");
		return ret;
	}

	/* following is setting value */

	ret = hd_audioout_set(p_init_info->audio_out_ctrl, HD_AUDIOOUT_PARAM_DRV_CONFIG, &dt_hdal_spec.audioout_drv_conf);
	if (ret != HD_OK) {
		printf("hd_audioout_set(HD_AUDIOOUT_PARAM_DRV_CONFIG) fail\n");
		return ret;
	}

	if ((ret = hd_audiocap_close(p_init_info->audio_cap_ctrl)) != HD_OK) {
		printf("hd_audiocap_close fail\n");
		goto ext;
	}
	if ((ret = hd_audioout_close(p_init_info->audio_out_ctrl)) != HD_OK) {
		printf("hd_audioout_close fail\n");
		goto ext;
	}

	if ((ret = hd_audiocap_uninit()) != HD_OK) {
		printf("hd_audiocap_uninit fail\n");
		goto ext;
	}
	if ((ret = hd_audioout_uninit()) != HD_OK) {
		printf("hd_audioout_uninit fail\n");
		goto ext;
	}
ext:
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common uninit fail\n");
	}

	return ret;
}


HD_RESULT clear_buffer_black(unsigned int buf_paddr, unsigned int buf_size, int pxlfmt, int width, int height)
{
	unsigned int i;
	void *p_fb0_va = 0;
	unsigned int *pbuf;

	p_fb0_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, buf_paddr, buf_size);
	if (p_fb0_va == NULL) {
		printf("hd_common_mem_mmap pa(0x%x) fail\n", (int)buf_paddr);
		return HD_ERR_NG;
	}
	pbuf = (unsigned int *)(p_fb0_va);
	if (pxlfmt == HD_VIDEO_PXLFMT_ARGB1555) {
		memset(pbuf, 0, buf_size);
	}  else if (pxlfmt == HD_VIDEO_PXLFMT_YUV420_NVX3) {
		////(c400 0000 0000 0000 0000 0000) y pattern for y_size
		////(e000 0000 0000 0000 0000 0000) uv pattern 		
		unsigned int y_size = ((width * height * 3) >> 2); 
		unsigned int yuv_size = ((width * height * 9) >> 3);//(w*h*3/2)*0.75
		unsigned int loop_cnt = (yuv_size >> 2);
		unsigned int buf_offset = 0;
		unsigned int buf_start = 0;
		unsigned int buf_len = 0;
		//fill first buf to yuv420_sce black
		for (i = 0; i < loop_cnt; i++) {
			buf_offset = ((i * 4) % yuv_size);		
			if (buf_offset < y_size) {
				pbuf[i++] = 0x000000c4;
				pbuf[i++] = 0;
				pbuf[i] = 0;
			} else {
				pbuf[i++] = 0x000000e0;
				pbuf[i++] = 0;
				pbuf[i] = 0;
			}
			
		}		
		buf_start = ALIGN256_UP(((unsigned int)pbuf + yuv_size));
		//copy first buf to others buf,that align to 256 start addr
		while(ALIGN256_UP(buf_start + yuv_size) < ((unsigned int)p_fb0_va + buf_size)) {		
			if((((unsigned int)p_fb0_va + buf_size) - buf_start) > yuv_size) {
				buf_len = yuv_size;
				memcpy((void *)buf_start, (void *)pbuf, buf_len);
			}
			buf_start = ALIGN256_UP(buf_start + yuv_size);	
		}
		
	}  else if (pxlfmt == HD_VIDEO_PXLFMT_YUV420) {		
		unsigned int y_size = (width * height); 
		unsigned int yuv_size = ((width * height * 3) >> 1);//(w*h*3/2)
		unsigned int loop_cnt = (yuv_size >> 2);
		unsigned int buf_offset = 0;
		unsigned int buf_start = 0;
		unsigned int buf_len = 0;
		//fill first buf to yuv420 black
		for (i = 0; i < loop_cnt; i++) {
			buf_offset = ((i * 4) % yuv_size);		
			if (buf_offset < y_size) {
				pbuf[i] = 0x10101010;			
			} else {
				pbuf[i] = 0x80808080;
			}
			
		}		
		buf_start = ALIGN256_UP(((unsigned int)pbuf + yuv_size));
		//copy first buf to others buf,that align to 256 start addr
		while(ALIGN256_UP(buf_start + yuv_size) < ((unsigned int)p_fb0_va + buf_size)) {		
			if((((unsigned int)p_fb0_va + buf_size) - buf_start) > yuv_size) {
				buf_len = yuv_size;
				memcpy((void *)buf_start, (void *)pbuf, buf_len);
			}
			buf_start = ALIGN256_UP(buf_start + yuv_size);	
		}
		
	} else { /*yuv422*/ 
		///unsigned int yuv_size = ALIGN16_UP(width) * ALIGN16_UP(height); /* internal buffer allocation is 16 alignment */
		for (i = 0; i < (buf_size >> 2); i++) {
			if (pxlfmt == HD_VIDEO_PXLFMT_YUV422_ONE) {
				pbuf[i] = 0x10801080;
			}
		}
	}

	if (HD_OK != hd_common_mem_munmap(p_fb0_va, buf_size)) {
		printf("hd_common_mem_munmap fail\n");
		return HD_ERR_NG;
	}
	return HD_OK;
}

/* LCD300: /dev/fb0,/dev/fb1,/dev/fb2 */
HD_RESULT videoout0_setup(HD_PATH_ID *ctrl_path, VIDEOOUT_INFO *videoout_0)
{
	HD_FB_ENABLE fb_cfg;
	UINT32 fb_buf_sz = 0, buf_size = 0, pool_fb_start = 0;
	HD_COMMON_MEM_POOL_INFO pool_disp0_fb;
	HD_COMMON_MEM_POOL_INFO pool_disp0_in;
	HD_VIDEOOUT_DEVNVR_CONFIG *devnvr_cfg = &videoout_0->dev_cfg;
	HD_FB_FMT fb_fmt;
	int fb_w, fb_h;
    int i;
	
	if (hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, ctrl_path) != HD_OK) {
		printf("Error hd_videoout_open\n");
		return HD_ERR_NG;
	}

	fb_w = devnvr_cfg->plane[0].max_w;
	fb_h = devnvr_cfg->plane[0].max_h;
	pool_disp0_fb.type = HD_COMMON_MEM_DISP0_FB_POOL;
	pool_disp0_fb.ddr_id = DDR_ID0;
	if (hd_common_mem_get(HD_COMMON_MEM_PARAM_POOL_CONFIG, (VOID *)&pool_disp0_fb) != HD_OK) {
		printf("Error hd_common_mem_get:pool_type %d ddr_id %d\r\n", pool_disp0_fb.type, pool_disp0_fb.ddr_id);
		goto error_return;
	}
	pool_fb_start = pool_disp0_fb.start_addr;

	pool_disp0_in.type = HD_COMMON_MEM_DISP0_IN_POOL;
	pool_disp0_in.ddr_id = DDR_ID0;
	if (hd_common_mem_get(HD_COMMON_MEM_PARAM_POOL_CONFIG, (VOID *)&pool_disp0_in) != HD_OK) {
		printf("Error hd_common_mem_get:pool_type %d ddr_id %d\r\n", pool_disp0_in.type, pool_disp0_in.ddr_id);
		goto error_return;
	}
	buf_size = pool_disp0_in.blk_cnt * pool_disp0_in.blk_size;
	
	if (videoout_0->fb_state[0].enable) {
		fb_buf_sz = (ALIGN16_UP(devnvr_cfg->plane[0].max_w) * ALIGN16_UP(devnvr_cfg->plane[0].max_h) * devnvr_cfg->plane[0].max_bpp) / 8;
		fb_buf_sz = ALIGN4096_UP(fb_buf_sz);
		if (fb_buf_sz > buf_size) {
			printf("Error disp0_in to set buffer %dx%d size %d (pool size %d)\n", fb_w, fb_h, (int)fb_buf_sz, (int)buf_size);
			goto error_return;
		}
		devnvr_cfg->plane[0].buf_paddr = pool_disp0_in.start_addr;
		devnvr_cfg->plane[0].ddr_id = pool_disp0_in.ddr_id;
		devnvr_cfg->plane[0].buf_len = fb_buf_sz;
		if (is_show_logo) {
			if (show_logo(devnvr_cfg->plane[0].buf_paddr, buf_size, videoout_0->fb_cfg[0].fmt, fb_w, fb_h) != HD_OK) {
				printf("paster logo to videoout0 fail\n");
				goto error_return;
			}
		} else {
			clear_buffer_black(devnvr_cfg->plane[0].buf_paddr, buf_size, videoout_0->fb_cfg[0].fmt, fb_w, fb_h); /* clear all the disp_in with black */
		}
	} else {
		devnvr_cfg->plane[0].max_w = devnvr_cfg->plane[0].max_h = 0;
	}
	if (videoout_0->fb_state[1].enable) {
		fb_buf_sz = (ALIGN16_UP(devnvr_cfg->plane[1].max_w) * ALIGN16_UP(devnvr_cfg->plane[1].max_h) * devnvr_cfg->plane[1].max_bpp) / 8;
		fb_buf_sz = ALIGN4096_UP(fb_buf_sz);
		devnvr_cfg->plane[1].buf_paddr = pool_fb_start;
		devnvr_cfg->plane[1].ddr_id = pool_disp0_fb.ddr_id;
		devnvr_cfg->plane[1].buf_len = fb_buf_sz;
		pool_fb_start += devnvr_cfg->plane[1].buf_len;
		clear_buffer_black(pool_disp0_fb.start_addr, (pool_disp0_fb.blk_size * pool_disp0_fb.blk_cnt),
						   videoout_0->fb_cfg[1].fmt, 0, 0); /* clear all the disp_fb with black */
	} else {
		devnvr_cfg->plane[1].max_w = devnvr_cfg->plane[1].max_h = 0;
	}
	if (devnvr_cfg->plane[1].gui_rld_enable) {
		devnvr_cfg->plane[1].rle_buf_paddr = pool_fb_start;
		devnvr_cfg->plane[1].rle_buf_len = fb_buf_sz;
		pool_fb_start += devnvr_cfg->plane[1].rle_buf_len;
	} 
	
	if (videoout_0->fb_state[2].enable) {
		fb_buf_sz = (ALIGN16_UP(devnvr_cfg->plane[2].max_w) * ALIGN16_UP(devnvr_cfg->plane[2].max_h) * devnvr_cfg->plane[2].max_bpp) / 8;
		fb_buf_sz = ALIGN4096_UP(fb_buf_sz);
		devnvr_cfg->plane[2].buf_paddr = pool_fb_start;
		devnvr_cfg->plane[2].ddr_id = pool_disp0_fb.ddr_id;
		devnvr_cfg->plane[2].buf_len = fb_buf_sz;
		pool_fb_start += devnvr_cfg->plane[2].buf_len;
	} else {
		devnvr_cfg->plane[2].max_w = devnvr_cfg->plane[2].max_h = 0;
	}
	if (pool_fb_start > pool_disp0_fb.start_addr + (pool_disp0_fb.blk_size * pool_disp0_fb.blk_cnt)) { /* check over size */
		printf("videoout0 fb1-fb2 size: fb1(%dKB), fb1_rle(%dKB), fb2(%dKB) needs %dKB (only %dKB)\n",
			   devnvr_cfg->plane[1].buf_len / 1024, devnvr_cfg->plane[1].rle_buf_len / 1024, devnvr_cfg->plane[2].buf_len / 1024,
			   (devnvr_cfg->plane[1].buf_len + devnvr_cfg->plane[1].rle_buf_len + devnvr_cfg->plane[2].buf_len) / 1024,
			   (int)(pool_disp0_fb.blk_size * pool_disp0_fb.blk_cnt) / 1024);
		goto error_return;
	}
	if (hd_videoout_set(*ctrl_path, HD_VIDEOOUT_PARAM_DEV_CONFIG, devnvr_cfg) != HD_OK) {
		printf("Error hd_videoout_set: video_0_ctrl fail HD_VIDEOOUT_PARAM_DEV_CONFIG\n");
		goto error_return;
	}

	/* set fb0~fb2 cfg */
	for (i = 0; i < 3; i++) {		
		fb_fmt.fb_id = videoout_0->fb_cfg[i].fb_id;
		if (devnvr_cfg->plane[i].gui_rld_enable == 1 && videoout_0->fb_cfg[i].fmt == HD_VIDEO_PXLFMT_ARGB1555) {
			fb_fmt.fmt = HD_VIDEO_PXLFMT_ARGB1555_RLE;
		} else {
			fb_fmt.fmt = videoout_0->fb_cfg[i].fmt;
		}
		if (hd_videoout_set(*ctrl_path, HD_VIDEOOUT_PARAM_FB_FMT, &fb_fmt) != HD_OK) {
			printf("hd_videoout_set:videoout_0,pathid(%#lx),HD_VIDEOOUT_PARAM_FB_FMT,fb%d fail\n", *ctrl_path, fb_fmt.fb_id);
			goto error_return;
		}
		fb_cfg.fb_id = videoout_0->fb_state[i].fb_id;
		fb_cfg.enable = videoout_0->fb_state[i].enable;
		if (hd_videoout_set(*ctrl_path, HD_VIDEOOUT_PARAM_FB_ENABLE, &fb_cfg) != HD_OK) {
			printf("hd_videoout_set:videoout_0,pathid(%#lx),HD_VIDEOOUT_PARAM_FB_ENABLE,fb%d fail\n", *ctrl_path, fb_cfg.fb_id);
			goto error_return;
		}
	}

	if (hd_videoout_close(*ctrl_path) != HD_OK) {
		printf("Error hd_videoout_close\n");
		return HD_ERR_NG;
	}
	return HD_OK;

error_return:
	if (hd_videoout_close(*ctrl_path) != HD_OK) {
		printf("Error hd_videoout_close\n");
		return HD_ERR_NG;
	}
	return HD_ERR_NG;
}


/* LCD200: /dev/fb3,/dev/fb4,/dev/fb5 */
HD_RESULT videoout1_setup(HD_PATH_ID *ctrl_path, VIDEOOUT_INFO *videoout_1)
{
	HD_FB_ENABLE fb_cfg;
	UINT32 fb_buf_sz = 0, buf_size = 0, pool_fb_start = 0;
	HD_COMMON_MEM_POOL_INFO pool_disp1_fb;
	HD_COMMON_MEM_POOL_INFO pool_disp1_in;
	HD_VIDEOOUT_DEVNVR_CONFIG *devnvr_cfg = &videoout_1->dev_cfg;
	HD_FB_FMT fb_fmt;
	HD_FB_ATTR fb_attr;
	HD_RESULT ret;
	int fb_w, fb_h;
	int i;
	
	if (hd_videoout_open(0, HD_VIDEOOUT_1_CTRL, ctrl_path) != HD_OK) {
		printf("Error hd_videoout_open\n");
		return HD_ERR_NG;
	}

	fb_w = devnvr_cfg->plane[0].max_w;
	fb_h = devnvr_cfg->plane[0].max_h;

	pool_disp1_fb.type = HD_COMMON_MEM_DISP1_FB_POOL;
	pool_disp1_fb.ddr_id = DDR_ID0;
	if (hd_common_mem_get(HD_COMMON_MEM_PARAM_POOL_CONFIG, (VOID *)&pool_disp1_fb) != HD_OK) {
		printf("Error hd_common_mem_get:pool_type %d ddr_id %d\r\n", pool_disp1_fb.type, pool_disp1_fb.ddr_id);
		goto error_return;
	}
	pool_fb_start = pool_disp1_fb.start_addr;

	pool_disp1_in.type = HD_COMMON_MEM_DISP1_IN_POOL;
	pool_disp1_in.ddr_id = DDR_ID0;
	if (hd_common_mem_get(HD_COMMON_MEM_PARAM_POOL_CONFIG, (VOID *)&pool_disp1_in) != HD_OK) {
		printf("Error hd_common_mem_get:pool_type %d ddr_id %d\r\n", pool_disp1_in.type, pool_disp1_in.ddr_id);
		goto error_return;
	}
	buf_size = pool_disp1_in.blk_cnt * pool_disp1_in.blk_size;
	if (videoout_1->fb_state[0].enable) {
		fb_buf_sz = (ALIGN16_UP(devnvr_cfg->plane[0].max_w) * ALIGN16_UP(devnvr_cfg->plane[0].max_h) * devnvr_cfg->plane[0].max_bpp) / 8;
		fb_buf_sz = ALIGN4096_UP(fb_buf_sz);
		if (fb_buf_sz > buf_size) {
			printf("Error disp1_in to set buffer %dx%d size %d (pool size %d)\n", fb_w, fb_h, (int)fb_buf_sz, (int)buf_size);
			goto error_return;
		}
		devnvr_cfg->plane[0].buf_paddr = pool_disp1_in.start_addr;
		devnvr_cfg->plane[0].ddr_id = pool_disp1_in.ddr_id;
		devnvr_cfg->plane[0].buf_len = fb_buf_sz;
		
		if (is_show_logo) {
			if (show_logo(devnvr_cfg->plane[0].buf_paddr, buf_size, videoout_1->fb_cfg[0].fmt, fb_w, fb_h) != HD_OK) {
				printf("paster logo to videoout1 fail\n");
				goto error_return;
			}
		} else {
			clear_buffer_black(devnvr_cfg->plane[0].buf_paddr, buf_size, videoout_1->fb_cfg[0].fmt, fb_w, fb_h); /* clear all the disp_in with black */
		}
	
	} else {
		devnvr_cfg->plane[0].max_w = devnvr_cfg->plane[0].max_h = 0;
	}
	if (videoout_1->fb_state[1].enable) {
		fb_buf_sz = (ALIGN16_UP(devnvr_cfg->plane[1].max_w) * ALIGN16_UP(devnvr_cfg->plane[1].max_h) * devnvr_cfg->plane[1].max_bpp) / 8;
		fb_buf_sz = ALIGN4096_UP(fb_buf_sz);
		devnvr_cfg->plane[1].buf_paddr = pool_fb_start;
		devnvr_cfg->plane[1].ddr_id = pool_disp1_fb.ddr_id;
		devnvr_cfg->plane[1].buf_len = fb_buf_sz;
		pool_fb_start += devnvr_cfg->plane[1].buf_len;
		clear_buffer_black(pool_disp1_fb.start_addr, (pool_disp1_fb.blk_size * pool_disp1_fb.blk_cnt),
					   videoout_1->fb_cfg[1].fmt, 0, 0); /* clear all the disp_fb with black */
	} else {
		devnvr_cfg->plane[1].max_w = devnvr_cfg->plane[1].max_h = 0;
	}
	if (videoout_1->fb_state[2].enable) {
		fb_buf_sz = (ALIGN16_UP(devnvr_cfg->plane[2].max_w) * ALIGN16_UP(devnvr_cfg->plane[2].max_h) * devnvr_cfg->plane[2].max_bpp) / 8;
		fb_buf_sz = ALIGN4096_UP(fb_buf_sz);
		devnvr_cfg->plane[2].buf_paddr = pool_fb_start;
		devnvr_cfg->plane[2].ddr_id = pool_disp1_fb.ddr_id;
		devnvr_cfg->plane[2].buf_len = fb_buf_sz;
		pool_fb_start += devnvr_cfg->plane[2].buf_len;
		
	} else {
		devnvr_cfg->plane[2].max_w = devnvr_cfg->plane[2].max_h = 0;
	}
	if (pool_fb_start > pool_disp1_fb.start_addr + (pool_disp1_fb.blk_size * pool_disp1_fb.blk_cnt)) { /* check over size */
		printf("videoout1 fb4-fb5 size: fb4(%dKB), fb4_rle(%dKB), fb5(%dKB) needs %dKB (only %dKB)\n",
			   devnvr_cfg->plane[1].buf_len / 1024, devnvr_cfg->plane[1].rle_buf_len / 1024, devnvr_cfg->plane[2].buf_len / 1024,
			   (devnvr_cfg->plane[1].buf_len + devnvr_cfg->plane[1].rle_buf_len + devnvr_cfg->plane[2].buf_len) / 1024,
			   (int)(pool_disp1_fb.blk_size * pool_disp1_fb.blk_cnt) / 1024);
		goto error_return;
	}
	
	if (hd_videoout_set(*ctrl_path, HD_VIDEOOUT_PARAM_DEV_CONFIG, devnvr_cfg) != HD_OK) {
		printf("Error hd_videoout_set: video_1_ctrl fail HD_VIDEOOUT_PARAM_DEV_CONFIG\n");
		goto error_return;
	}
	//修改lcd200 的blend設定讓gui 預設能顯示
	fb_attr.fb_id = HD_FB0;
    ret = hd_videoout_get(*ctrl_path, HD_VIDEOOUT_PARAM_FB_ATTR, &fb_attr);
    if (ret != HD_OK) {
        printf("hd_videoout_get:HD_VIDEOOUT_PARAM_FB_ATTR, fail\n");
        return ret;
    }
    fb_attr.alpha_1555 = 0;
    fb_attr.alpha_blend = 0;
    ret = hd_videoout_set(*ctrl_path, HD_VIDEOOUT_PARAM_FB_ATTR, &fb_attr);
    if (ret != HD_OK) {
        printf("hd_videoout_set:HD_VIDEOOUT_PARAM_FB_ATTR fb_id(%d) fail\n", fb_attr.fb_id);
        return ret;
    }
	fb_attr.fb_id = HD_FB1;
    ret = hd_videoout_get(*ctrl_path, HD_VIDEOOUT_PARAM_FB_ATTR, &fb_attr);
    if (ret != HD_OK) {
        printf("hd_videoout_get:HD_VIDEOOUT_PARAM_FB_ATTR, fail\n");
        return ret;
    }
    fb_attr.alpha_1555 = 0;
    fb_attr.alpha_blend = 255;
    ret = hd_videoout_set(*ctrl_path, HD_VIDEOOUT_PARAM_FB_ATTR, &fb_attr);
    if (ret != HD_OK) {
        printf("hd_videoout_set:HD_VIDEOOUT_PARAM_FB_ATTR fb_id(%d) fail\n", fb_attr.fb_id);
        return ret;
    }
	/* set fb0~fb2 cfg , Only Plane_1 can set state*/
	for (i = 0; i < 3; i++) {		
		fb_fmt.fb_id = videoout_1->fb_cfg[i].fb_id;
		fb_fmt.fmt = videoout_1->fb_cfg[i].fmt;
		if (hd_videoout_set(*ctrl_path, HD_VIDEOOUT_PARAM_FB_FMT, &fb_fmt) != HD_OK) {
			printf("hd_videoout_set:videoout_1,pathid(%#lx),HD_VIDEOOUT_PARAM_FB_FMT,fb%d fail\n", *ctrl_path, fb_fmt.fb_id);
			goto error_return;
		}
		fb_cfg.fb_id = videoout_1->fb_state[i].fb_id;
		fb_cfg.enable = videoout_1->fb_state[i].enable;
		if ((fb_cfg.fb_id == HD_FB1) && hd_videoout_set(*ctrl_path, HD_VIDEOOUT_PARAM_FB_ENABLE, &fb_cfg) != HD_OK) {
			printf("hd_videoout_set:videoout_1,pathid(%#lx),HD_VIDEOOUT_PARAM_FB_ENABLE,fb%d fail\n", *ctrl_path, fb_cfg.fb_id);
			goto error_return;
		}
	}

	if (hd_videoout_close(*ctrl_path) != HD_OK) {
		printf("Error hd_videoout_close\n");
		goto error_return;
	}
	return HD_OK;

error_return:
	if (hd_videoout_close(*ctrl_path) != HD_OK) {
		printf("Error hd_videoout_close\n");
		return HD_ERR_NG;
	}
	return HD_ERR_NG;
}


HD_RESULT videoout_module_init(MODULE_INIT_INFO *p_init_info)
{

	if (hd_common_init(1) != HD_OK) {
		printf("common init fail\n");
		return HD_ERR_NG;
	}
	if (hd_videoout_init() != HD_OK) {
		printf("Error hd_videoout_init\n");
		return HD_ERR_NG;
	}
	if (dt_hdal_spec.videoout_0.dev_cfg.chip_state == 1) {
		if (videoout0_setup(&p_init_info->video_0_ctrl, &dt_hdal_spec.videoout_0) != HD_OK) {
			printf("videoout0_init fail\n");
			return HD_ERR_NG;
		}
	}

	if (dt_hdal_spec.videoout_1.dev_cfg.chip_state == 1) {
		if (videoout1_setup(&p_init_info->video_1_ctrl, &dt_hdal_spec.videoout_1) != HD_OK) {
			printf("videoout1_init fail\n");
			return HD_ERR_NG;
		}
	}

	if (hd_videoout_uninit() != HD_OK) {
		printf("Error hd_videoout_uninit\n");
		return HD_ERR_NG;
	}
	if (hd_common_uninit() != HD_OK) {
		printf("common uninit fail\n");
		return HD_ERR_NG;
	}
	return HD_OK;
}

int assign_pool_addr(void)
{
	int i, sys_fd, ddr_id;
	unsigned int ddr_paddr[DDR_ID_MAX], ddr_total[DDR_ID_MAX], ddr_usage[DDR_ID_MAX];
	unsigned int pool_size = 0;
	struct nvtmem_hdal_base sys_hdal;

	memset(&sys_hdal, 0, sizeof(sys_hdal));
	sys_fd = open("/dev/nvtmem0", O_RDWR);
	if (sys_fd < 0) {
		printf("Error: cannot open /dev/nvtmem0 device.\n");
		exit(0);
	}
	if (ioctl(sys_fd, NVTMEM_GET_DTS_HDAL_BASE, &sys_hdal) < 0) {
		printf("PCIE_SYS_IOC_HDALBASE! \n");
		close(sys_fd);
		exit(0);
	}
	close(sys_fd);

	memset(ddr_paddr, 0, sizeof(ddr_paddr));
	memset(ddr_total, 0, sizeof(ddr_total));
	memset(ddr_usage, 0, sizeof(ddr_usage));

	for (ddr_id = 0; ddr_id < DDR_ID_MAX; ddr_id++) {
		if (sys_hdal.base[ddr_id]) {
			printf("DDR%d: hdal_mem 0x%08X, 0x%08X\n", ddr_id, sys_hdal.base[ddr_id], sys_hdal.size[ddr_id]);
		}
		if (sys_hdal.base[ddr_id] == 0) {
			continue;
		}
		ddr_paddr[ddr_id] = sys_hdal.base[ddr_id];
		ddr_total[ddr_id] = sys_hdal.size[ddr_id];

		for (i = 0; i < sizeof(dt_hdal_spec.mem_pool_info) / sizeof(HD_COMMON_MEM_POOL_INFO); i++) {
			if (dt_hdal_spec.mem_pool_info[i].ddr_id != ddr_id) {
				continue;
			}
			if (dt_hdal_spec.mem_pool_info[i].ddr_id == DDR_ID_MAX) {
				break;
			}
			dt_hdal_spec.mem_pool_info[i].start_addr = ddr_paddr[ddr_id];
			dt_hdal_spec.mem_pool_info[i].blk_size = ALIGN4096_UP(dt_hdal_spec.mem_pool_info[i].blk_size);  //for 4K page handling
			pool_size =  dt_hdal_spec.mem_pool_info[i].blk_size * dt_hdal_spec.mem_pool_info[i].blk_cnt;
			ddr_paddr[ddr_id] += pool_size;
			ddr_usage[ddr_id] += pool_size;
#if 0
			printf("HDAL DDR0 type %d start 0x%x size %dKB (%lu x %lu) total %dKB\n",
				   pool_info[i].type, (int)pool_info[i].start_addr, pool_size / 1024, pool_info[i].blk_size,
				   pool_info[i].blk_cnt, ddr_usage[ddr_id] / 1024);
			fflush(stdout);
			usleep(100 * 1000);
#endif
		}

		printf("### Hdal DDR%d Pool usage %dKB (Total %dKB Free %dKB)\n", ddr_id,
			   ddr_usage[ddr_id] / 1024, ddr_total[ddr_id] / 1024, (ddr_total[ddr_id] - ddr_usage[ddr_id]) / 1024);
		fflush(stdout);
		if (ddr_usage[ddr_id] > ddr_total[ddr_id]) {
			printf("DDR%d Pool total size overflow %dKB (hdal size %dKB)\n", ddr_id,
				   ddr_usage[ddr_id] / 1024, ddr_total[ddr_id] / 1024);
			return -1;
		}
	}
	return 0;
}

HD_RESULT mem_init(MODULE_INIT_INFO *p_init_info)
{
	HD_RESULT ret;

	memset(&p_init_info->mem_cfg, 0, sizeof(HD_COMMON_MEM_INIT_CONFIG));

	if (assign_pool_addr() < 0) {
		return -1;
	}

	ret = hd_common_init(1);
	if (ret != HD_OK) {
		printf("common init fail1\n");
		return -1;
	}

	memcpy(p_init_info->mem_cfg.pool_info, &dt_hdal_spec.mem_pool_info, sizeof(dt_hdal_spec.mem_pool_info));

	ret = hd_common_mem_init(&p_init_info->mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		return -1;
	}

	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("hd_common_uninit fail\n");
		return -1;
	}
	return 0;
}

unsigned int transfer_char_to_int(char *arry)
{
	return ((arry[0] << 24) | (arry[1] << 16) | (arry[2] << 8) | arry[3]);
}

int get_dts_int_node_prop(unsigned char *dts_dtb, int node_offset, char *name, unsigned int *prop_val)
{
	int len;
	const void *nodep;  /* property node pointer */
	char *p_prop;

	if (prop_val) {
		nodep = fdt_getprop(dts_dtb, node_offset, name, &len);
		if (len == 0 || !nodep) {
			printf("fdt_getprop:name(%s) len(%d) fail\n", name, len);
			return -1;
		}
		p_prop = (char *)nodep;
		*prop_val = transfer_char_to_int(p_prop);
		return len;
	} else {
		printf("Invalid prop_val pointer.\n");
		return -1;
	}
}

HD_COMMON_VIDEO_OUT_TYPE convert_str_to_HD_COMMON_VIDEO_OUT_TYPE(char *output_type)
{
	if (strncmp(__to_str(HD_COMMON_VIDEO_OUT_HDMI), output_type, strlen(__to_str(HD_COMMON_VIDEO_OUT_HDMI))) == 0) {
		return HD_COMMON_VIDEO_OUT_HDMI;
	} else if (strncmp(__to_str(HD_COMMON_VIDEO_OUT_VGA), output_type, strlen(__to_str(HD_COMMON_VIDEO_OUT_VGA))) == 0) {
		return HD_COMMON_VIDEO_OUT_VGA;
	} else if (strncmp(__to_str(HD_COMMON_VIDEO_OUT_CVBS), output_type, strlen(__to_str(HD_COMMON_VIDEO_OUT_CVBS))) == 0) {
		return HD_COMMON_VIDEO_OUT_CVBS;
	} else if (strncmp(__to_str(HD_COMMON_VIDEO_OUT_LCD), output_type, strlen(__to_str(HD_COMMON_VIDEO_OUT_LCD))) == 0) {
		return HD_COMMON_VIDEO_OUT_LCD;
	} else {
		return HD_COMMON_VIDEO_OUT_TYPE_NONE;
	}
}

HD_VIDEOOUT_INPUT_DIM convert_str_to_HD_VIDEOOUT_INPUT_DIM(char *input_dim)
{
	if (strncmp(__to_str(HD_VIDEOOUT_IN_720x480), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_720x480))) == 0) {
		return HD_VIDEOOUT_IN_720x480;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_720x576), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_720x576))) == 0) {
		return HD_VIDEOOUT_IN_720x576;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_1024x768), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_1024x768))) == 0) {
		return HD_VIDEOOUT_IN_1024x768;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_1280x720), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_1280x720))) == 0) {
		return HD_VIDEOOUT_IN_1280x720;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_1280x1024), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_1280x1024))) == 0) {
		return HD_VIDEOOUT_IN_1280x1024;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_1600x1200), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_1600x1200))) == 0) {
		return HD_VIDEOOUT_IN_1600x1200;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_2560x1440), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_2560x1440))) == 0) {
		return HD_VIDEOOUT_IN_2560x1440;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_1920x1080), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_1920x1080))) == 0) {
		return HD_VIDEOOUT_IN_1920x1080;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_3840x2160), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_3840x2160))) == 0) {
		return HD_VIDEOOUT_IN_3840x2160;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_1440x900), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_1440x900))) == 0) {
		return HD_VIDEOOUT_IN_1440x900;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_1680x1050), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_1680x1050))) == 0) {
		return HD_VIDEOOUT_IN_1680x1050;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_800x600), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_800x600))) == 0) {
		return HD_VIDEOOUT_IN_800x600;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_640x480), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_640x480))) == 0) {
		return HD_VIDEOOUT_IN_640x480;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_2560x720), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_2560x720))) == 0) {
		return HD_VIDEOOUT_IN_2560x720;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_1920x1200), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_1920x1200))) == 0) {
		return HD_VIDEOOUT_IN_1920x1200;
	} else if (strncmp(__to_str(HD_VIDEOOUT_IN_3840x1080), input_dim, strlen(__to_str(HD_VIDEOOUT_IN_3840x1080))) == 0) {
		return HD_VIDEOOUT_IN_3840x1080;
	} else {
		return HD_VIDEOOUT_IN_AUTO;
	}
}

HD_VIDEOOUT_HDMI_ID convert_str_to_HD_VIDEOOUT_HDMI_ID(char *hdmi_id)
{
	if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1280X720P50), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1280X720P50))) == 0) {
		return HD_VIDEOOUT_HDMI_1280X720P50;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1280X720P60), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1280X720P60))) == 0) {
		return HD_VIDEOOUT_HDMI_1280X720P60;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1920X1080P30), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1920X1080P30))) == 0) {
		return HD_VIDEOOUT_HDMI_1920X1080P30;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1920X1080P60), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1920X1080P60))) == 0) {
		return HD_VIDEOOUT_HDMI_1920X1080P60;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_3840X2160P30), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_3840X2160P30))) == 0) {
		return HD_VIDEOOUT_HDMI_3840X2160P30;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1024X768P60), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1024X768P60))) == 0) {
		return HD_VIDEOOUT_HDMI_1024X768P60;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1280X1024P30), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1280X1024P30))) == 0) {
		return HD_VIDEOOUT_HDMI_1280X1024P30;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1280X1024P60), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1280X1024P60))) == 0) {
		return HD_VIDEOOUT_HDMI_1280X1024P60;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1600X1200P30), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1600X1200P30))) == 0) {
		return HD_VIDEOOUT_HDMI_1600X1200P30;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1600X1200P60), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1600X1200P60))) == 0) {
		return HD_VIDEOOUT_HDMI_1600X1200P60;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_2560X1440P30), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_2560X1440P30))) == 0) {
		return HD_VIDEOOUT_HDMI_2560X1440P30;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_2560X1440P60), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_2560X1440P60))) == 0) {
		return HD_VIDEOOUT_HDMI_2560X1440P60;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1920X1080P50), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1920X1080P50))) == 0) {
		return HD_VIDEOOUT_HDMI_1920X1080P50;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_3840X2160P25), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_3840X2160P25))) == 0) {
		return HD_VIDEOOUT_HDMI_3840X2160P25;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1440X900P60), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1440X900P60))) == 0) {
		return HD_VIDEOOUT_HDMI_1440X900P60;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_720X480P60), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_720X480P60))) == 0) {
		return HD_VIDEOOUT_HDMI_720X480P60;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_720X576P50), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_720X576P50))) == 0) {
		return HD_VIDEOOUT_HDMI_720X576P50;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1920X1080P25), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1920X1080P25))) == 0) {
		return HD_VIDEOOUT_HDMI_1920X1080P25;
	} else if (strncmp(__to_str(HD_VIDEOOUT_HDMI_1920X1080P30), hdmi_id, strlen(__to_str(HD_VIDEOOUT_HDMI_1920X1080P30))) == 0) {
		return HD_VIDEOOUT_HDMI_1920X1080P30;
	} else {
		printf("Not support hdmi_id(%s)\r\n", hdmi_id);
		return HD_VIDEOOUT_HDMI_NO_CHANGE;
	}
}

HD_VIDEOOUT_VGA_ID convert_str_to_HD_VIDEOOUT_VGA_ID(char *vga_id)
{
	if (strncmp(__to_str(HD_VIDEOOUT_VGA_720X480), vga_id, strlen(__to_str(HD_VIDEOOUT_VGA_720X480))) == 0) {
		return HD_VIDEOOUT_VGA_720X480;
	} else if (strncmp(__to_str(HD_VIDEOOUT_VGA_1024X768), vga_id, strlen(__to_str(HD_VIDEOOUT_VGA_1024X768))) == 0) {
		return HD_VIDEOOUT_VGA_1024X768;
	} else if (strncmp(__to_str(HD_VIDEOOUT_VGA_1280X720), vga_id, strlen(__to_str(HD_VIDEOOUT_VGA_1280X720))) == 0) {
		return HD_VIDEOOUT_VGA_1280X720;
	} else if (strncmp(__to_str(HD_VIDEOOUT_VGA_1280X1024), vga_id, strlen(__to_str(HD_VIDEOOUT_VGA_1280X1024))) == 0) {
		return HD_VIDEOOUT_VGA_1280X1024;
	} else if (strncmp(__to_str(HD_VIDEOOUT_VGA_1600X1200), vga_id, strlen(__to_str(HD_VIDEOOUT_VGA_1600X1200))) == 0) {
		return HD_VIDEOOUT_VGA_1600X1200;
	} else if (strncmp(__to_str(HD_VIDEOOUT_VGA_1920X1080), vga_id, strlen(__to_str(HD_VIDEOOUT_VGA_1920X1080))) == 0) {
		return HD_VIDEOOUT_VGA_1920X1080;
	} else if (strncmp(__to_str(HD_VIDEOOUT_VGA_1920X1200), vga_id, strlen(__to_str(HD_VIDEOOUT_VGA_1920X1200))) == 0) {
		return HD_VIDEOOUT_VGA_1920X1200;
	} else if (strncmp(__to_str(HD_VIDEOOUT_VGA_1680X1050), vga_id, strlen(__to_str(HD_VIDEOOUT_VGA_1680X1050))) == 0) {
		return HD_VIDEOOUT_VGA_1680X1050;
	} else if (strncmp(__to_str(VENDOR_VIDEOOUT_VGA_800X600), vga_id, strlen(__to_str(VENDOR_VIDEOOUT_VGA_800X600))) == 0) {
		return VENDOR_VIDEOOUT_VGA_800X600;
	} else {
		printf("Not support vga_id(%s)\r\n", vga_id);
		return HD_VIDEOOUT_VGA_MAX;
	}
}

HD_VIDEOOUT_CVBS_ID convert_str_to_HD_VIDEOOUT_CVBS_ID(char *cvbs_id)
{
	if (strncmp(__to_str(HD_VIDEOOUT_CVBS_NTSC), cvbs_id, strlen(__to_str(HD_VIDEOOUT_CVBS_NTSC))) == 0) {
		return HD_VIDEOOUT_CVBS_NTSC;
	} else if (strncmp(__to_str(HD_VIDEOOUT_CVBS_PAL), cvbs_id, strlen(__to_str(HD_VIDEOOUT_CVBS_PAL))) == 0) {
		return HD_VIDEOOUT_CVBS_PAL;
	} else {
		printf("Not support cvbs_id(%s)\r\n", cvbs_id);
		return HD_VIDEOOUT_CVBS_MAX;
	}
}


HD_VIDEOOUT_LCD_ID convert_str_to_HD_VIDEOOUT_LCD_ID(char *lcd_id)
{
	if (strcmp(__to_str(VENDOR_VIDEOOUT_BT1120_1920X1080P), lcd_id) == 0) {
		return VENDOR_VIDEOOUT_BT1120_1920X1080P;
	} else if (strcmp(__to_str(VENDOR_VIDEOOUT_BT565_1280X720P), lcd_id) == 0) {
		return VENDOR_VIDEOOUT_BT565_1280X720P;
	}  else {
		printf("Not support lcd_id(%s)\r\n", lcd_id);
		return HD_VIDEOOUT_LCD_0;
	}
}

HD_VIDEO_PXLFMT convert_str_to_HD_VIDEO_PXLFMT(char *buf_fmt)
{
	if (strcmp(__to_str(HD_VIDEO_PXLFMT_I4), buf_fmt) == 0) {
		return HD_VIDEO_PXLFMT_I4;
	} else if (strcmp(__to_str(HD_VIDEO_PXLFMT_I2), buf_fmt) == 0) {
		return HD_VIDEO_PXLFMT_I2;
	} else if (strcmp(__to_str(HD_VIDEO_PXLFMT_ARGB8888), buf_fmt) == 0) {
		return HD_VIDEO_PXLFMT_ARGB8888;
	} else if (strcmp(__to_str(HD_VIDEO_PXLFMT_ARGB1555), buf_fmt) == 0) {
		return HD_VIDEO_PXLFMT_ARGB1555;
	} else if (strcmp(__to_str(HD_VIDEO_PXLFMT_YUV422_ONE), buf_fmt) == 0) {
		return HD_VIDEO_PXLFMT_YUV422_ONE;
	} else if (strcmp(__to_str(HD_VIDEO_PXLFMT_YUV422_NVX3), buf_fmt) == 0) {
		return HD_VIDEO_PXLFMT_YUV422_NVX3;
	} else if (strcmp(__to_str(HD_VIDEO_PXLFMT_YUV420_NVX3), buf_fmt) == 0) {
		return HD_VIDEO_PXLFMT_YUV420_NVX3;		
	} else if (strcmp(__to_str(HD_VIDEO_PXLFMT_YUV420), buf_fmt) == 0) {
		return HD_VIDEO_PXLFMT_YUV420;
	}  else {
		printf("Not support buf_fmt(%s)\r\n", buf_fmt);
		return HD_VIDEO_PXLFMT_NONE;
	}
}

int parse_videoout0_node(unsigned char *buf, int node_offset)
{
	//unsigned int prop_val[2];
	char prop_str[DT_NODE_SZ];
	unsigned int prop_val;
	unsigned int val[2];
	int sub_node_offset;
	int ret = 0;
	const void *nodep;
	char *p_prop;
	int len;
	int i;
	char sub_node_name[16] = {0};
	char homology_dev[128] = {0};
	int offset = 0;

	if (node_offset < 0) {
		printf("check node_offset fail\n");
		return -1;
	}
	sub_node_offset = fdt_subnode_offset(buf, node_offset, "videoout0");
	if (sub_node_offset < 0) {
		printf("fdt_subnode_offset fail\n");
		return -1;
	}

	nodep = fdt_getprop(buf, sub_node_offset, "mode", &len);
	if (len == 0 || !nodep) {
		printf("fdt_getprop:name(mode) len(%d) (%s)fail\n", len, nodep);
		return -1;
	}	
	p_prop = (char *)nodep;
	if (strlen(p_prop) > DT_NODE_SZ) {
		printf("%s:%d:check strlen fail\n", __func__, __LINE__);
		return -1;
	}
	strcpy(prop_str, p_prop);
	dt_hdal_spec.videoout_0.dev_cfg.mode.output_type = convert_str_to_HD_COMMON_VIDEO_OUT_TYPE(prop_str);
	p_prop += strlen(prop_str) + 1;
	if (strlen(p_prop) > DT_NODE_SZ) {
		printf("%s:%d:check strlen fail\n", __func__, __LINE__);
		return -1;
	}
	strcpy(prop_str, p_prop);
	dt_hdal_spec.videoout_0.dev_cfg.mode.input_dim = convert_str_to_HD_VIDEOOUT_INPUT_DIM(prop_str);
	p_prop += strlen(prop_str) + 1;
	if (strlen(p_prop) > DT_NODE_SZ) {
		printf("%s:%d:check strlen fail\n", __func__, __LINE__);
		return -1;
	}
	strcpy(prop_str, p_prop);
	if (dt_hdal_spec.videoout_0.dev_cfg.mode.output_type == HD_COMMON_VIDEO_OUT_HDMI) {
		dt_hdal_spec.videoout_0.dev_cfg.mode.output_mode.hdmi = convert_str_to_HD_VIDEOOUT_HDMI_ID(prop_str);
	} else if (dt_hdal_spec.videoout_0.dev_cfg.mode.output_type == HD_COMMON_VIDEO_OUT_VGA) {
		dt_hdal_spec.videoout_0.dev_cfg.mode.output_mode.vga = convert_str_to_HD_VIDEOOUT_VGA_ID(prop_str);
	} else if (dt_hdal_spec.videoout_0.dev_cfg.mode.output_type == HD_COMMON_VIDEO_OUT_LCD) {
		dt_hdal_spec.videoout_0.dev_cfg.mode.output_mode.lcd = convert_str_to_HD_VIDEOOUT_LCD_ID(prop_str);
	} else {
		printf("videoout0 only support to HDMI/VGA output\n");
		return -1;
	}

	nodep = fdt_getprop(buf, sub_node_offset, "homology", &len);
	if (len == 0 || !nodep) {
		printf("fdt_getprop:name(homology) len(%d) fail\n", len);
		return -1;
	}	
	p_prop = (char *)nodep;	
	offset = 0;
	dt_hdal_spec.videoout_0.dev_cfg.homology = 0;
	while (len) {
		if (offset >= len) {
			break;
		}
		if (strlen(p_prop) > DT_NODE_SZ) {
			printf("%s:%d:check strlen fail\n", __func__, __LINE__);
			return -1;
		}
		strcpy(prop_str, p_prop);
		memcpy(&homology_dev[offset], p_prop, strlen(prop_str));
		if (strlen(prop_str)) {
			dt_hdal_spec.videoout_0.dev_cfg.homology |= (1 << convert_str_to_HD_COMMON_VIDEO_OUT_TYPE(prop_str));
		}
		offset += (strlen(prop_str) + 1);
		p_prop += offset;
		homology_dev[offset - 1] = ',';
	}
	if (offset - 1)
		homology_dev[offset - 1] = '\0';
	else
		homology_dev[offset] = '\0';
	if (get_dts_int_node_prop(buf, sub_node_offset, "chip_state", &prop_val) < 0) {
		printf("get videoout0 chip_state value fail\n");
		return -1;
	}
	dt_hdal_spec.videoout_0.dev_cfg.chip_state = prop_val;

	if (get_dts_int_node_prop(buf, sub_node_offset, "gui_rld_enable", &prop_val) < 0) {
		printf("get videoout0 gui_rld_enable value fail\n");
		return -1;
	}
	dt_hdal_spec.videoout_0.dev_cfg.plane[1].gui_rld_enable = prop_val;
	for (i = 0; i < 3; i++) {
		sprintf(sub_node_name, "fb%d_dim", i);
		nodep = fdt_getprop(buf, sub_node_offset, sub_node_name, &len);
		if (len == 0 || !nodep) {
			printf("fdt_getprop:name(%s) len(%d) fail\n", sub_node_name, len);
			return -1;
		}
		p_prop = (char *)nodep;
		val[0] = transfer_char_to_int(p_prop);
		val[1] = transfer_char_to_int(p_prop + sizeof(unsigned int));
		dt_hdal_spec.videoout_0.dev_cfg.plane[i].max_w = val[0];
		dt_hdal_spec.videoout_0.dev_cfg.plane[i].max_h = val[1];

		sprintf(sub_node_name, "fb%d_state", i);
		nodep = fdt_getprop(buf, sub_node_offset, sub_node_name, &len);	
		if (len == 0 || !nodep) {
			printf("fdt_getprop:name(%s) len(%d) fail\n", sub_node_name, len);
			return -1;
		}
		p_prop = (char *)nodep;
		prop_val = transfer_char_to_int(p_prop);
		dt_hdal_spec.videoout_0.fb_state[i].enable = prop_val;
		if(i == 0) {
			dt_hdal_spec.videoout_0.fb_state[i].fb_id = HD_FB0;
			dt_hdal_spec.videoout_0.fb_cfg[i].fb_id = HD_FB0;
		} else if (i == 1) {
			dt_hdal_spec.videoout_0.fb_state[i].fb_id = HD_FB1;
			dt_hdal_spec.videoout_0.fb_cfg[i].fb_id = HD_FB1;
		} else  {
			dt_hdal_spec.videoout_0.fb_state[i].fb_id = HD_FB2;
			dt_hdal_spec.videoout_0.fb_cfg[i].fb_id = HD_FB2;
		}		

		sprintf(sub_node_name, "fb%d_fmt", i);
		nodep = fdt_getprop(buf, sub_node_offset, sub_node_name, &len);		
		if (len == 0 || !nodep) {
			printf("fdt_getprop:name(%s) len(%d) fail\n", sub_node_name, len);
			return -1;
		}
		p_prop = (char *)nodep;
		if (strlen(p_prop) > DT_NODE_SZ) {
			printf("%s:%d:check strlen fail\n", __func__, __LINE__);
			return -1;
		}
		strcpy(prop_str, p_prop);		
		dt_hdal_spec.videoout_0.fb_cfg[i].fmt = convert_str_to_HD_VIDEO_PXLFMT(prop_str);

		sprintf(sub_node_name, "fb%d_bpp", i);
		nodep = fdt_getprop(buf, sub_node_offset, sub_node_name, &len);
		if (len == 0 || !nodep) {
			printf("fdt_getprop:name(%s) len(%d) fail\n", sub_node_name, len);
			return -1;
		}
		p_prop = (char *)nodep;
		prop_val = transfer_char_to_int(p_prop);
		dt_hdal_spec.videoout_0.dev_cfg.plane[i].max_bpp = prop_val;
	}
	if (dump_dts_info) {
		printf("videoout_0 info:\n");
		printf("    output_typ=%d,input_dim=%d,output_mode=%d,homology=\"%s\",chip_state=%d,gui_rld_enable=%d\n", \
			   dt_hdal_spec.videoout_0.dev_cfg.mode.output_type, dt_hdal_spec.videoout_0.dev_cfg.mode.input_dim, \
			   dt_hdal_spec.videoout_0.dev_cfg.mode.output_type == HD_COMMON_VIDEO_OUT_HDMI ? \
			   dt_hdal_spec.videoout_0.dev_cfg.mode.output_mode.hdmi : dt_hdal_spec.videoout_0.dev_cfg.mode.output_mode.vga, \
			   homology_dev, dt_hdal_spec.videoout_0.dev_cfg.chip_state, \
			   dt_hdal_spec.videoout_0.dev_cfg.plane[1].gui_rld_enable);
		for (i = 0; i < 3; i++) {
			printf("    fb%d_info:max_w=%d,max_h=%d,fb_id=%d, fmt=%#x,max_bpp=%d,state=%d\n", \
				   i, dt_hdal_spec.videoout_0.dev_cfg.plane[i].max_w, dt_hdal_spec.videoout_0.dev_cfg.plane[i].max_h, \
				   dt_hdal_spec.videoout_0.fb_cfg[i].fb_id, dt_hdal_spec.videoout_0.fb_cfg[i].fmt, \
				   dt_hdal_spec.videoout_0.dev_cfg.plane[i].max_bpp, dt_hdal_spec.videoout_0.fb_state[i].enable);
		}
		fflush(stdout);
	}
	return ret;
}

int parse_videoout1_node(unsigned char *buf, int node_offset)
{
	//unsigned int prop_val[2];
	char prop_str[DT_NODE_SZ];
	unsigned int prop_val;
	unsigned int val[2];
	int sub_node_offset;
	int ret = 0;
	const void *nodep;
	char *p_prop;
	int len;
	int i;
	char homology_dev[128] = {0};
	char sub_node_name[16] = {0};
	int offset = 0;

	if (node_offset < 0) {
		printf("check node_offset fail\n");
		return -1;
	}
	sub_node_offset = fdt_subnode_offset(buf, node_offset, "videoout1");
	if (sub_node_offset < 0) {
		printf("fdt_subnode_offset fail\n");
		return -1;
	}

	nodep = fdt_getprop(buf, sub_node_offset, "mode", &len);
	if (len == 0 || !nodep) {
		printf("fdt_getprop:name(mode) len(%d) fail\n", len);
		return -1;
	}
	p_prop = (char *)nodep;
	if (strlen(p_prop) > DT_NODE_SZ) {
		printf("%s:%d:check strlen fail\n", __func__, __LINE__);
		return -1;
	}
	strcpy(prop_str, p_prop);
	dt_hdal_spec.videoout_1.dev_cfg.mode.output_type = convert_str_to_HD_COMMON_VIDEO_OUT_TYPE(prop_str);
	p_prop += strlen(prop_str) + 1;
	if (strlen(p_prop) > DT_NODE_SZ) {
		printf("%s:%d:check strlen fail\n", __func__, __LINE__);
		return -1;
	}
	strcpy(prop_str, p_prop);
	dt_hdal_spec.videoout_1.dev_cfg.mode.input_dim = convert_str_to_HD_VIDEOOUT_INPUT_DIM(prop_str);
	p_prop += strlen(prop_str) + 1;
	if (strlen(p_prop) > DT_NODE_SZ) {
		printf("%s:%d:check strlen fail\n", __func__, __LINE__);
		return -1;
	}
	strcpy(prop_str, p_prop);
	if (dt_hdal_spec.videoout_1.dev_cfg.mode.output_type == HD_COMMON_VIDEO_OUT_VGA) {
		dt_hdal_spec.videoout_1.dev_cfg.mode.output_mode.vga = convert_str_to_HD_VIDEOOUT_VGA_ID(prop_str);
	} else if (dt_hdal_spec.videoout_1.dev_cfg.mode.output_type == HD_COMMON_VIDEO_OUT_CVBS) {
		dt_hdal_spec.videoout_1.dev_cfg.mode.output_mode.cvbs = convert_str_to_HD_VIDEOOUT_CVBS_ID(prop_str);
	}  else if (dt_hdal_spec.videoout_1.dev_cfg.mode.output_type == HD_COMMON_VIDEO_OUT_LCD) {
		dt_hdal_spec.videoout_1.dev_cfg.mode.output_mode.lcd = convert_str_to_HD_VIDEOOUT_LCD_ID(prop_str);
	} else {
		printf("videoout1 only support to VGA/CVBS output\n");
		return -1;
	}
	nodep = fdt_getprop(buf, sub_node_offset, "homology", &len);
	if (len == 0 || !nodep) {
			printf("fdt_getprop:name(homology) len(%d) fail\n", len);
			return -1;
	}
	p_prop = (char *)nodep;
	offset = 0;
	dt_hdal_spec.videoout_1.dev_cfg.homology = 0;
	while (len) {
		if (offset >= len) {
			break;
		}
		if (strlen(p_prop) > DT_NODE_SZ) {
			printf("%s:%d:check strlen fail\n", __func__, __LINE__);
			return -1;
		}
		strcpy(prop_str, p_prop);
		memcpy(&homology_dev[offset], p_prop, strlen(prop_str));
		if (strlen(prop_str)) {
			dt_hdal_spec.videoout_1.dev_cfg.homology |= (1 << convert_str_to_HD_COMMON_VIDEO_OUT_TYPE(prop_str));
		}
		offset += (strlen(prop_str) + 1);
		p_prop += offset;
		homology_dev[offset - 1] = ',';
	}
	if (offset - 1)
		homology_dev[offset - 1] = '\0';
	else 
		homology_dev[offset] = '\0';
	if (get_dts_int_node_prop(buf, sub_node_offset, "chip_state", &prop_val) < 0) {
		printf("get videoout1 chip_state value fail\n");
		return -1;
	}
	dt_hdal_spec.videoout_1.dev_cfg.chip_state = prop_val;

	for (i = 0; i < 3; i++) {
		sprintf(sub_node_name, "fb%d_dim", i);
		nodep = fdt_getprop(buf, sub_node_offset, sub_node_name, &len);
		if (len == 0 || !nodep) {
			printf("fdt_getprop:name(%s) len(%d) fail\n", sub_node_name, len);
			return -1;
		}
		p_prop = (char *)nodep;
		val[0] = transfer_char_to_int(p_prop);
		val[1] = transfer_char_to_int(p_prop + sizeof(unsigned int));
		dt_hdal_spec.videoout_1.dev_cfg.plane[i].max_w = val[0];
		dt_hdal_spec.videoout_1.dev_cfg.plane[i].max_h = val[1];

		sprintf(sub_node_name, "fb%d_state", i);
		nodep = fdt_getprop(buf, sub_node_offset, sub_node_name, &len);
		if (len == 0 || !nodep) {
			printf("fdt_getprop:name(%s) len(%d) fail\n", sub_node_name, len);
			return -1;
		}
		p_prop = (char *)nodep;
		prop_val = transfer_char_to_int(p_prop);
		dt_hdal_spec.videoout_1.fb_state[i].enable = prop_val;
		if(i == 0) {
			dt_hdal_spec.videoout_1.fb_state[i].fb_id = HD_FB0;
			dt_hdal_spec.videoout_1.fb_cfg[i].fb_id = HD_FB0;
		} else if (i == 1) {
			dt_hdal_spec.videoout_1.fb_state[i].fb_id = HD_FB1;
			dt_hdal_spec.videoout_1.fb_cfg[i].fb_id = HD_FB1;
		} else  {
			dt_hdal_spec.videoout_1.fb_state[i].fb_id = HD_FB2;
			dt_hdal_spec.videoout_1.fb_cfg[i].fb_id = HD_FB2;
		}			

		sprintf(sub_node_name, "fb%d_fmt", i);
		nodep = fdt_getprop(buf, sub_node_offset, sub_node_name, &len);
		if (len == 0 || !nodep) {
			printf("fdt_getprop:name(%s) len(%d) fail\n", sub_node_name, len);
			return -1;
		}
		p_prop = (char *)nodep;
		if (strlen(p_prop) > DT_NODE_SZ) {
			printf("%s:%d:check strlen fail\n", __func__, __LINE__);
			return -1;
		}
		strcpy(prop_str, p_prop);
		dt_hdal_spec.videoout_1.fb_cfg[i].fmt = convert_str_to_HD_VIDEO_PXLFMT(prop_str);

		sprintf(sub_node_name, "fb%d_bpp", i);
		nodep = fdt_getprop(buf, sub_node_offset, sub_node_name, &len);
		if (len == 0 || !nodep) {
			printf("fdt_getprop:name(%s) len(%d) fail\n", sub_node_name, len);
			return -1;
		}
		p_prop = (char *)nodep;
		prop_val = transfer_char_to_int(p_prop);
		dt_hdal_spec.videoout_1.dev_cfg.plane[i].max_bpp = prop_val;
	}
	if (dump_dts_info) {
		printf("videoout_1 info:\n");
		printf("    output_typ=%d,input_dim=%d,output_mode=%d,homology=\"%s\",chip_state=%d\n", \
			   dt_hdal_spec.videoout_1.dev_cfg.mode.output_type, dt_hdal_spec.videoout_1.dev_cfg.mode.input_dim, \
			   dt_hdal_spec.videoout_1.dev_cfg.mode.output_type == HD_COMMON_VIDEO_OUT_HDMI ? \
			   dt_hdal_spec.videoout_1.dev_cfg.mode.output_mode.hdmi : dt_hdal_spec.videoout_1.dev_cfg.mode.output_mode.vga, \
			   homology_dev, dt_hdal_spec.videoout_1.dev_cfg.chip_state);
		for (i = 0; i < 3; i++) {
			printf("    fb%d_info:max_w=%d,max_h=%d,fb_id=%d, fmt=%#x,max_bpp=%d,state=%d\n", \
				   i, dt_hdal_spec.videoout_1.dev_cfg.plane[i].max_w, dt_hdal_spec.videoout_1.dev_cfg.plane[i].max_h, \
				   dt_hdal_spec.videoout_1.fb_cfg[i].fb_id, dt_hdal_spec.videoout_1.fb_cfg[i].fmt, \
				   dt_hdal_spec.videoout_1.dev_cfg.plane[i].max_bpp, dt_hdal_spec.videoout_1.fb_state[i].enable);
		}
		fflush(stdout);
	}
	return ret;
}
int parse_lcd(unsigned char *buf, int spec_offset)
{
	int node_offset;
	//unsigned int prop_val;
	int ret = 0;
	//char node_name[DT_NODE_SZ] = {0};

	if (spec_offset < 0) {
		printf("chck spec_offset fail\n");
		return -1;
	}
	node_offset = fdt_subnode_offset(buf, spec_offset, "lcd_cfg");
	if (node_offset < 0) {
		printf("fdt_subnode_offset fail\n");
		return -1;
	}

	if (parse_videoout0_node(buf, node_offset) < 0) {
		printf("parse_videoout0_node fail\n");
		ret = -1;
		goto exit;
	}
	
	if (parse_videoout1_node(buf, node_offset) < 0) {
		printf("parse_videoout1_node fail\n");
		ret = -1;
		goto exit;
	}

exit:
	return ret;
}

HD_COMMON_MEM_POOL_TYPE convert_str_to_HD_COMMON_MEM_POOL_TYPE(char *pool_name)
{
	if (strncmp(__to_str(HD_COMMON_MEM_COMMON_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_COMMON_POOL))) == 0) {
		return HD_COMMON_MEM_COMMON_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP0_IN_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP0_IN_POOL))) == 0) {
		return HD_COMMON_MEM_DISP0_IN_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP1_IN_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP1_IN_POOL))) == 0) {
		return HD_COMMON_MEM_DISP1_IN_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP2_IN_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP2_IN_POOL))) == 0) {
		return HD_COMMON_MEM_DISP2_IN_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP3_IN_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP3_IN_POOL))) == 0) {
		return HD_COMMON_MEM_DISP3_IN_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP4_IN_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP4_IN_POOL))) == 0) {
		return HD_COMMON_MEM_DISP4_IN_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP5_IN_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP5_IN_POOL))) == 0) {
		return HD_COMMON_MEM_DISP5_IN_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP0_CAP_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP0_CAP_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP0_CAP_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP1_CAP_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP1_CAP_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP1_CAP_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP2_CAP_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP2_CAP_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP2_CAP_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP3_CAP_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP3_CAP_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP3_CAP_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP4_CAP_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP4_CAP_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP4_CAP_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP5_CAP_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP5_CAP_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP5_CAP_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP0_ENC_SCL_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP0_ENC_SCL_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP0_ENC_SCL_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP1_ENC_SCL_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP1_ENC_SCL_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP1_ENC_SCL_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP2_ENC_SCL_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP2_ENC_SCL_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP2_ENC_SCL_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP3_ENC_SCL_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP3_ENC_SCL_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP3_ENC_SCL_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP4_ENC_SCL_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP4_ENC_SCL_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP4_ENC_SCL_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP5_ENC_SCL_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP5_ENC_SCL_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP5_ENC_SCL_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP0_ENC_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP0_ENC_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP0_ENC_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP1_ENC_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP1_ENC_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP1_ENC_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP2_ENC_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP2_ENC_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP2_ENC_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP3_ENC_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP3_ENC_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP3_ENC_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP4_ENC_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP4_ENC_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP4_ENC_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP5_ENC_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP5_ENC_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP5_ENC_OUT_POOL;
	}  else if (strncmp(__to_str(HD_COMMON_MEM_DISP_DEC_IN_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP_DEC_IN_POOL))) == 0) {
		return HD_COMMON_MEM_DISP_DEC_IN_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP_DEC_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP_DEC_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_DISP_DEC_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP_DEC_OUT_RATIO_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP_DEC_OUT_RATIO_POOL))) == 0) {
		return HD_COMMON_MEM_DISP_DEC_OUT_RATIO_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_ENC_CAP_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_ENC_CAP_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_ENC_CAP_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_ENC_SCL_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_ENC_SCL_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_ENC_SCL_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_ENC_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_ENC_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_ENC_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_AU_ENC_AU_GRAB_OUT_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_AU_ENC_AU_GRAB_OUT_POOL))) == 0) {
		return HD_COMMON_MEM_AU_ENC_AU_GRAB_OUT_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_AU_DEC_AU_RENDER_IN_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_AU_DEC_AU_RENDER_IN_POOL))) == 0) {
		return HD_COMMON_MEM_AU_DEC_AU_RENDER_IN_POOL;
	}  else if (strncmp(__to_str(HD_COMMON_MEM_ENC_REF_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_ENC_REF_POOL))) == 0) {
		return HD_COMMON_MEM_ENC_REF_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DEC_TILE_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DEC_TILE_POOL))) == 0) {
		return HD_COMMON_MEM_DEC_TILE_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_FLOW_MD_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_FLOW_MD_POOL))) == 0) {
		return HD_COMMON_MEM_FLOW_MD_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_GLOBAL_MD_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_GLOBAL_MD_POOL))) == 0) {
		return HD_COMMON_MEM_GLOBAL_MD_POOL;
	}  else if (strncmp(__to_str(HD_COMMON_MEM_TMNR_MOTION_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_TMNR_MOTION_POOL))) == 0) {
		return HD_COMMON_MEM_TMNR_MOTION_POOL;
	}  else if (strncmp(__to_str(HD_COMMON_MEM_OSG_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_OSG_POOL))) == 0) {
		return HD_COMMON_MEM_OSG_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_GFX_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_GFX_POOL))) == 0) {
		return HD_COMMON_MEM_GFX_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DSP_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DSP_POOL))) == 0) {
		return HD_COMMON_MEM_DSP_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_CNN_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_CNN_POOL))) == 0) {
		return HD_COMMON_MEM_CNN_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP0_FB_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP0_FB_POOL))) == 0) {
		return HD_COMMON_MEM_DISP0_FB_POOL;
	}  else if (strncmp(__to_str(HD_COMMON_MEM_DISP1_FB_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP1_FB_POOL))) == 0) {
		return HD_COMMON_MEM_DISP1_FB_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP2_FB_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP2_FB_POOL))) == 0) {
		return HD_COMMON_MEM_DISP2_FB_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP3_FB_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP3_FB_POOL))) == 0) {
		return HD_COMMON_MEM_DISP3_FB_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP4_FB_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP4_FB_POOL))) == 0) {
		return HD_COMMON_MEM_DISP4_FB_POOL;
	} else if (strncmp(__to_str(HD_COMMON_MEM_DISP5_FB_POOL), pool_name, strlen(__to_str(HD_COMMON_MEM_DISP5_FB_POOL))) == 0) {
		return HD_COMMON_MEM_DISP5_FB_POOL;
	}  else if (strncmp(__to_str(HD_COMMON_MEM_USER_BLK), pool_name, strlen(__to_str(HD_COMMON_MEM_USER_BLK))) == 0) {
		return HD_COMMON_MEM_USER_BLK;
	} else {
		printf("Undefine pool(%s)\n", pool_name);
		return HD_COMMON_MEM_USER_DEFINIED_POOL;
	}
	return -1;
}

/*<ddr_id>,"pool_type",<blk_size>,<cnt>,<shared_pool> */
int prase_mem_pool(unsigned char *buf, int spec_offset)
{
	int node_offset;
	char node_name[DT_NODE_SZ] = {0};
	int ret = 0;
	const void *nodep;
	unsigned int prop_val;
	char prop_str[64];
	char pool_name[64];
	char *p_prop;
	int len;
	int offset = 0;
	int i = 0;
	int share_pool_idx = 0;
	
	if (spec_offset < 0) {
		printf("check spec_offset fail\n");
		return -1;
	}
	node_offset = fdt_subnode_offset(buf, spec_offset, "mem_pool");
	if (node_offset < 0) {
		printf("fdt_subnode_offset fail\n");
		return -1;
	}
	sprintf(node_name, "pool");
	nodep = fdt_getprop(buf, node_offset, node_name, &len);
	if (len == 0 || !nodep) {
		printf("fdt_getprop:name(%s) len(%d) fail\n", node_name, len);
		return i;
	}
	p_prop = (char *)nodep;
	if (dump_dts_info) {
		printf("pool info:\n");
	}
	while (i < HD_COMMON_MEM_MAX_POOL_NUM) {
		if (offset >= len) {
			break;
		}
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.mem_pool_info[i].ddr_id = prop_val;
		if (strlen(p_prop + offset) > DT_NODE_SZ) {
			printf("%s:%d:check strlen fail\n", __func__, __LINE__);
			return -1;
		}
		strcpy(prop_str, p_prop + offset);
		offset += (strlen(prop_str) + 1);
		if (dump_dts_info) {
			strcpy(pool_name, prop_str);
		}
		dt_hdal_spec.mem_pool_info[i].type = convert_str_to_HD_COMMON_MEM_POOL_TYPE(prop_str);
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.mem_pool_info[i].blk_size = (prop_val * 1024);
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.mem_pool_info[i].blk_cnt = prop_val;
		if (strlen(p_prop + offset) > DT_NODE_SZ) {
			printf("%s:%d:check strlen fail\n", __func__, __LINE__);
			return -1;
		}
		strcpy(prop_str, p_prop + offset);
		offset += (strlen(prop_str) + 1);
		if (strlen(prop_str)) {			
			for (share_pool_idx = 0; share_pool_idx < COMMON_MEM_SHARED_POOL_NUM; share_pool_idx++) {
				dt_hdal_spec.mem_pool_info[i].shared_pool[0] |= SET_SHAREPOOL_VAL(convert_str_to_HD_COMMON_MEM_POOL_TYPE(prop_str), share_pool_idx);
				strcpy(prop_str, p_prop + offset);
				if (!strlen(prop_str)) {//only one pool to share
					break;
				} else {
					offset += (strlen(prop_str) + 1);
				}
			}			
		} else {
			dt_hdal_spec.mem_pool_info[i].shared_pool[0] = 0;
		}
		dt_hdal_spec.mem_pool_info[i].start_addr = 0;
		if (dump_dts_info) {
			printf("    ddrid=%d,type=%03d,pool(%s),blk_size=%ldKB,blk_cnt=%ld,share_pool=(%#x)\n", dt_hdal_spec.mem_pool_info[i].ddr_id, \
				   dt_hdal_spec.mem_pool_info[i].type, pool_name, dt_hdal_spec.mem_pool_info[i].blk_size / 1024,
				   dt_hdal_spec.mem_pool_info[i].blk_cnt, dt_hdal_spec.mem_pool_info[i].shared_pool[0]);
		}
		i++;
	}
	if (i >= HD_COMMON_MEM_MAX_POOL_NUM) {
		printf("pool number(%d) exceed HD_COMMON_MEM_MAX_POOL_NUM(%d)\r\n",
			   i, HD_COMMON_MEM_MAX_POOL_NUM);
		return -1;
	}
	if (dump_dts_info) {
		fflush(stdout);
		sleep(1);
	}
	return ret;
}

/*<ssp_num>,<enable>,<ssp_chan>,<sample_size>,<sample_rate>,<ssp_clock>,<bit_clock>,<ssp_master>,<live_sound_ch> */
int prase_audio_in(unsigned char *buf, int spec_offset)
{
	int node_offset;
	char node_name[DT_NODE_SZ] = {0};
	int ret = 0;
	const void *nodep;
	unsigned int prop_val;
	char *p_prop;
	int offset = 0;
	int len;
	int i = 0;

	if (spec_offset < 0) {
		printf("check spec_offset fail\n");
		return -1;
	}
	node_offset = fdt_subnode_offset(buf, spec_offset, "audio_in");
	if (node_offset < 0) {
		printf("fdt_subnode_offset fail\n");
		return -1;
	}
	sprintf(node_name, "ssp");
	nodep = fdt_getprop(buf, node_offset, node_name, &len);
	if (len == 0 || !nodep) {
		printf("fdt_getprop:name(%s) len(%d) fail\n", node_name, len);
		return i;
	}
	p_prop = (char *)nodep;
	if (dump_dts_info) {
		printf("ssp info:\n");
	}
	while (i < AUDIOCAP_MAX_DEVICE_NUM) {
		if (offset >= len) {
			break;
		}
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audio_drv_cfg.ssp_config.ssp_num[i] = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audio_drv_cfg.ssp_config.enable[i] = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audio_drv_cfg.ssp_config.ssp_chan[i] = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audio_drv_cfg.ssp_config.sample_size[i] = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audio_drv_cfg.ssp_config.sample_rate[i] = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audio_drv_cfg.ssp_config.ssp_clock[i] = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audio_drv_cfg.ssp_config.bit_clock[i] = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audio_drv_cfg.ssp_config.ssp_master[i] = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audio_drv_cfg.ssp_config.live_sound_ch[i] = prop_val;
		if (dump_dts_info) {
			printf("    %d,num(%d),enable(%d),ssp_chan(%d),sample_size(%d),sample_rate(%d),ssp_clock(%d),bit_clock(%d),ssp_master(%d),live_sound_ch(%d)\n", \
				   i, dt_hdal_spec.audio_drv_cfg.ssp_config.ssp_num[i], dt_hdal_spec.audio_drv_cfg.ssp_config.enable[i], \
				   dt_hdal_spec.audio_drv_cfg.ssp_config.ssp_chan[i], dt_hdal_spec.audio_drv_cfg.ssp_config.sample_size[i], \
				   dt_hdal_spec.audio_drv_cfg.ssp_config.sample_rate[i], dt_hdal_spec.audio_drv_cfg.ssp_config.ssp_clock[i], \
				   dt_hdal_spec.audio_drv_cfg.ssp_config.bit_clock[i], dt_hdal_spec.audio_drv_cfg.ssp_config.ssp_master[i], \
				   dt_hdal_spec.audio_drv_cfg.ssp_config.live_sound_ch[i]);
			fflush(stdout);
		}
		i++;
		//exit:
	}
	return ret;
}

/*<enable>,<resample_ratio>,<playback_chmap> */
int prase_audio_out(unsigned char *buf, int spec_offset)
{
	int node_offset;
	char node_name[DT_NODE_SZ] = {0};
	int ret = 0;
	const void *nodep;
	unsigned int prop_val;
	char *p_prop;
	int len;
	int offset = 0;
	int i = 0;

	if (spec_offset < 0) {
		printf("check spec_offset fail\n");
		return -1;
	}
	node_offset = fdt_subnode_offset(buf, spec_offset, "audio_out");
	if (node_offset < 0) {
		printf("fdt_subnode_offset fail\n");
		return -1;
	}
	sprintf(node_name, "ssp");
	nodep = fdt_getprop(buf, node_offset, node_name, &len);
	if (len == 0 || !nodep) {
		printf("fdt_getprop:name(%s) len(%d) fail\n", node_name, len);
		return i;
	}
	p_prop = (char *)nodep;
	if (dump_dts_info) {
		printf("audio_out info:\n");
	}
	while (i < AUDIOCAP_MAX_DEVICE_NUM) {
		if (offset >= len) {
			break;
		}
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audioout_drv_conf.ssp_config.enable[i] = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audioout_drv_conf.ssp_config.resample_ratio[i] = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.audioout_drv_conf.ssp_config.playback_chmap[i] = prop_val;
		if (dump_dts_info) {
			printf("    %d,enable(%d),resample_ratio(%d),playback_chmap(%d)\n", i, dt_hdal_spec.audioout_drv_conf.ssp_config.enable[i], \
				   dt_hdal_spec.audioout_drv_conf.ssp_config.resample_ratio[i], dt_hdal_spec.audioout_drv_conf.ssp_config.playback_chmap[i]);
			fflush(stdout);
		}
		i++;
		//exit:
	}
	return ret;
}

/*<stereo>*/
int prase_audio_48k(unsigned char *buf, int spec_offset)
{
	int node_offset;
	char node_name[DT_NODE_SZ] = {0};
	int ret = 0;
	const void *nodep;
	unsigned int prop_val;
	char *p_prop;
	int len;
	int offset = 0;

	if (spec_offset < 0) {
		printf("check spec_offset fail\n");
	}
	node_offset = fdt_subnode_offset(buf, spec_offset, "audio_48k");
	if (node_offset < 0) {
		dt_hdal_spec.audio_drv_cfg.mono = 1; // Set default value
		return -1;
	}
	sprintf(node_name, "stereo");
	nodep = fdt_getprop(buf, node_offset, node_name, &len);
	if (len == 0 || !nodep) {
		printf("fdt_getprop:name(%s) len(%d) fail\n", node_name, len);
		return -1;
	}
	p_prop = (char *)nodep;
	if (dump_dts_info) {
		printf("audio_48k info:\n");
	}

	if (offset >= len) {
		return -1;
	}
	prop_val = transfer_char_to_int(p_prop + offset);
	//offset += sizeof(prop_val);
	if (prop_val)
		dt_hdal_spec.audio_drv_cfg.mono = 0;
	else
		dt_hdal_spec.audio_drv_cfg.mono = 1;
	if (dump_dts_info) {
		printf("    stereo(%d)\n", dt_hdal_spec.audio_drv_cfg.mono);
		fflush(stdout);
	}
	return ret;
}

/*<ssp_num>,<enable>,<ssp_chan>,<sample_size>,<sample_rate>,<ssp_clock>,<bit_clock>,<ssp_master>,<live_sound_ch> */
int prase_cap_md(unsigned char *buf, int spec_offset)
{
	int node_offset;
	char node_name[DT_NODE_SZ] = {0};
	int ret = 0;
	const void *nodep;
	unsigned int prop_val;
	char *p_prop;
	int offset = 0;
	int len;
	int i = 0;

	if (spec_offset < 0) {
		printf("echk spec_offset fail\n");
		return -1;
	}
	node_offset = fdt_subnode_offset(buf, spec_offset, "capture");
	if (node_offset < 0) {
		printf("fdt_subnode_offset fail\n");
		return -1;
	}
	sprintf(node_name, "md");
	nodep = fdt_getprop(buf, node_offset, node_name, &len);
	if (len == 0 || !nodep) {
		printf("fdt_getprop:name(%s) len(%d) fail\n", node_name, len);
		return i;
	}
	p_prop = (char *)nodep;
	while (i < AUDIOCAP_MAX_DEVICE_NUM) {
		if (offset >= len) {
			break;
		}
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.vcap_host.md.enable = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.vcap_host.md.mb_x_num_max = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.vcap_host.md.mb_y_num_max = prop_val;
		prop_val = transfer_char_to_int(p_prop + offset);
		offset += sizeof(prop_val);
		dt_hdal_spec.vcap_host.md.buf_src = prop_val;
		if (dump_dts_info) {
			printf("capture info:\n");
			printf("    md_enable(%d),md_x_max_nu(%d),md_y_max_nu(%d),md_src_buf(%d)\n", \
				   dt_hdal_spec.vcap_host.md.enable, dt_hdal_spec.vcap_host.md.mb_x_num_max, \
				   dt_hdal_spec.vcap_host.md.mb_y_num_max, dt_hdal_spec.vcap_host.md.buf_src);
			fflush(stdout);
		}
		i++;
		//exit:
	}
	return ret;
}


int parse_dt_base(char *dtsi_path)
{
	FILE *fp;
	unsigned char *buf;
	struct fdt_header fdt_hdr = {0};
	int spec_offset;
	int fdt_size;

	memset(&dt_hdal_spec, 0, sizeof(dt_hdal_spec));
	// load dtb to memory
	if (dtsi_path) {
		fp = fopen(dtsi_path, "rb");
		if (fp == NULL) {
			printf("failed to open %s\n", dtsi_path);
			return -1;
		}
		printf("open dtsi file(%s)\n", dtsi_path);
	} else {
		fp = fopen(DTB_PATH, "rb");
		if (fp == NULL) {
			printf("failed to open %s\n", DTB_PATH);
			return -1;
		}
		printf("open dtsi file(%s)\n", DTB_PATH);
	}

	if (fread(&fdt_hdr, 1, sizeof(fdt_hdr), fp) != sizeof(fdt_hdr)) {
		printf("%s: fread_1 failed.\n", __func__);
	}
	fseek(fp, 0, SEEK_SET);

	fdt_size = fdt_totalsize(&fdt_hdr);
	buf = (unsigned char *)malloc(fdt_totalsize(&fdt_hdr));

	if(fread(buf, 1, fdt_size, fp) != fdt_size) {
		printf("%s: fread_2 failed.\n", __func__);
	}
	fclose(fp);

	spec_offset = fdt_path_offset(buf, "/hdal-spec");
	if (spec_offset < 0) {
		free(buf);
		printf("parse hdal-spec fail\n");
		return -1;
	}

	//<parse lcd_cfg node
	if (parse_lcd(buf, spec_offset) < 0) {
		free(buf);
		printf("parse_lcd_dts fail\n");
		return -1;
	}

	//<parse mem_pool node
	if (prase_mem_pool(buf, spec_offset) < 0) {
		free(buf);
		printf("prase_mem_pool_dts fail\n");
		return -1;
	}

	//<parse audio node
	if (prase_audio_in(buf, spec_offset) < 0) {
		free(buf);
		printf("prase_audio_in_dts fail\n");
		return -1;
	}

	//<parse audio_out node
	if (prase_audio_out(buf, spec_offset) < 0) {
		free(buf);
		printf("prase_audio_out_dts fail\n");
		return -1;
	}

	//<parse capture node
	if (prase_cap_md(buf, spec_offset) < 0) {
		free(buf);
		printf("prase_capture_dts fail\n");
		return -1;
	}

	//<parse audio_48k node
	if (prase_audio_48k(buf, spec_offset) < 0) {
		printf("has no audio_48k setting, it's okay.\n");
	}

	printf("parse_dt_base OK\n");
	free(buf);
	return 0;
}


int main(int argc, char *argv[])
{
	HD_RESULT hdal_ret = HD_OK;
	INT ret = 0;
	MODULE_INIT_INFO mod_init_info;
	MODULE_TP28XX_INFO mod_init_info_tp;
	MODULE_NVP61XX_INFO mod_init_info_nvp;
	CHAR platform_str[8];

	if (argc == 3) {
		dump_dts_info = atoi(argv[2]);
	}

	if (argc == 7) {
		dump_dts_info = atoi(argv[2]);
		is_init_videocap = atoi(argv[3]);
		is_init_videoout = atoi(argv[4]);
		is_init_audio = atoi(argv[5]);
		is_show_logo = atoi(argv[6]);
	} 

	if (argv[1]) {
		if (parse_dt_base(argv[1]) < 0) {
			printf("parse_dt_base fail\n");
			return -1;
		}
	} else {
		if (parse_dt_base(NULL) < 0) {
			printf("parse_dt_base fail\n");
			return -1;
		}
	}

	if (is_init_videocap) {  	// videocap module insert.
		strcpy(platform_str, "DVR");
	} else {									// videocap module does not insert.
		strcpy(platform_str, "NVR");
	}

	printf("\nmodule_init %s (%s)\n", MODULE_VERSION, platform_str);
	fflush(stdout);

	/* memory init */
	hdal_ret = mem_init(&mod_init_info);
	if (hdal_ret != HD_OK) {
		printf("mem_init fail!\n");
		return -1;
	} else printf("mem_init OK\n");

	if (is_init_audio) {
		hdal_ret = audio_module_init(&mod_init_info);
		if (hdal_ret != HD_OK) {
			printf("audio_module_init fail!\n");
			return -1;
		}
	}	

	if (is_init_videoout) {
		hdal_ret = videoout_module_init(&mod_init_info);
		if (hdal_ret != HD_OK) {
			printf("videoout_module_init fail!\n");
			return -1;
		} else printf("videoout_module_init OK!\n");
	}

	
	if (is_init_videocap == 1) { //techpoint
		hdal_ret = videocap_module_init_tp(&mod_init_info_tp, AD_DEV_NODE_NAME);
		if (hdal_ret != HD_OK) {
			printf("videocap_module_init fail!\n");
			return -1;
		}
	} else if (is_init_videocap == 2) { //nextchip
		hdal_ret = videocap_module_init_nvp(&mod_init_info_nvp, AD_DEV_NODE_NAME_NVP);
		if (hdal_ret != HD_OK) {
			printf("videocap_module_init fail!\n");
			return -1;
		}
	}
	printf("module_init ok~\n");
	return ret;
}
