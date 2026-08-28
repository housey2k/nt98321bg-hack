#boot_ver=v1.5
#chipver=`head -1 /proc/pmu/chipver`
#chipid=`echo $chipver | cut -c 1-4`
echo -----------------------------------------------------------
echo "        Boot NVR_4CH"
echo -----------------------------------------------------------
echo "/sbin/mdev" > /proc/sys/kernel/hotplug

MODEL_PATH=$(dirname $MODEL)
MODEL_NAME="${MODEL_PATH##*/}"
echo This model is $MODEL_NAME

modprobe nvt_dmasys
modprobe log log_ksize=4096 crash_notify=/mnt/mtd/crash.sh	#crash to execute /mnt/mtd/crash.sh
echo /tmp > /proc/videograph/dumplog		#change log path to /tmp

modprobe ms max_channels=4
modprobe em max_channels=4

modprobe lcd_codec
modprobe nvt_tve100
modprobe nvt_hdmi20
modprobe flcd300-common
modprobe flcd300-pip suspend_state=1
modprobe flcd200-common
modprobe flcd200-pip suspend_state=1

modprobe vpe321_drv
modprobe kflow_vpe mod_init=1 max_chip_num=1 max_eng_num=1 max_minor_num=255 max_md_lv_num=16 max_total_cam_ch=25

modprobe nvt_audio
modprobe kflow_audio

#modprobe dei321_drv
#modprobe kflow_di max_chip_num=1 max_eng_num=1 max_minor_num=64 max_total_cam_ch=16

modprobe kdrv_jpg
modprobe h26xenc h26x_enc_max_width=2560 h26x_enc_max_height=1440 max_total_cam_ch=17
modprobe nvt_vencrc
modprobe kflow_videoenc
modprobe h26xdec h26xd_max_width=2560 h26xd_max_height=1440 max_total_cam_ch=12
modprobe kflow_videodec

modprobe kdrv_osg
modprobe kflow_osg

modprobe gm2d
modprobe gs max_channels=4
modprobe usr_proc
modprobe vpd quiet=0 max_channels=4
modprobe ddr_arb
echo doing mdev-s
mdev -s
echo done!!

# Syntax: module_init  [dtb_file] [is_dump_dts] [is_init_videocap] [is_init_videoout] [is_init_audio] [is_show_logo]
/mnt/mtd/module_init /mnt/mtd/cfg_NVR_4CH.dtb 0 0 1 1 0 &


# ch0: MCP100, VCAP316_M2, OSG, VPE316
# ch1: LCD, HEAVYLOAD
# ch2: VDEC
# ch3: VPE536, SSCA200, HEAVYLOAD_2
# ch4: CNN, NUE, NUE2, TSDE, HEAVLOAD_1
# ch5: CPU, DSP, DMA030
# ch6: VENC
# ch7: VCAP316_0, VCAP316_1
# cmd: echo w [grant/pri/dynamic_pri] [ch] [grant_count/priority/dynamic_pri] > /proc/nvt_drv_sys/dram_info
#      ch:0~7, grant_count:0~15, priority:0->high, 1->mid, 2->low, dynamic_pri:0->disable, 1-> mid, 2->high

echo w pri 5 0 > /proc/nvt_drv_sys/dram_info  # Set DDR CPU(ch5) high priority
echo w dynamic_pri 1 2 > /proc/nvt_drv_sys/dram_info  # Set DDR LCD(ch1) dynamic high priority

echo w grant 0 2 > /proc/nvt_drv_sys/dram_info  # set DDR DDR JPG,OSG,VPE316(ch0) grant 2
echo w grant 1 2 > /proc/nvt_drv_sys/dram_info  # set DDR LCD(ch1) grant 2
echo w grant 2 8 > /proc/nvt_drv_sys/dram_info  # set DDR VDEC(ch2) grant 8
echo w grant 3 2 > /proc/nvt_drv_sys/dram_info  # set DDR VPE536(ch3) grant 2
echo w grant 4 2 > /proc/nvt_drv_sys/dram_info  # set DDR CNN,NUE(ch4) grant 2
echo w grant 5 2 > /proc/nvt_drv_sys/dram_info  # set DDR CPU,DSP(ch5) grant 2
echo w grant 6 2 > /proc/nvt_drv_sys/dram_info  # set DDR VENC(ch6) grant 2
echo w grant 7 2 > /proc/nvt_drv_sys/dram_info  # set DDR VCAP(ch7) grant 2
