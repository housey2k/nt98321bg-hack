#boot_ver=v1.5
#chipver=`head -1 /proc/pmu/chipver`
#chipid=`echo $chipver | cut -c 1-4`
NVR_MODE=0
if [ -w /mnt/mtd/Config/HvrMode ];then
	NVR_MODE=`awk '$1 == "Digital-nDigiTalChn:" {print $2}' /mnt/mtd/Config/HvrMode`
fi
if [ -w /mnt/mtd/Config/PrvMode ];then
	NVR_MODE=`awk '$1 == "Digital-nDigiTalChn:" {print $2}' /mnt/mtd/Config/PrvMode`
fi

echo -----------------------------------------------------------
echo "        Boot NVR_MODE=$NVR_MODE"
echo -----------------------------------------------------------
echo "/sbin/mdev" > /proc/sys/kernel/hotplug

insmod vos/kwrap/kwrap.ko
#insmod kernel/drivers/usb/host/ehci-hcd.ko
insmod hdal/comm/nvt_dmasys/nvt_dmasys.ko
insmod hdal/comm/nvtmem/nvtmem.ko
if [ $NVR_MODE == 0 ];then
	insmod hdal/comm/util/log.ko log_ksize=64 crash_notify=/mnt/mtd/crash.sh	#crash to execute /mnt/mtd/crash.sh
else
	insmod hdal/comm/util/log.ko log_ksize=256 crash_notify=/mnt/mtd/crash.sh	#crash to execute /mnt/mtd/crash.sh
fi
echo /var > /proc/videograph/dumplog		#change log path to /tmp

if [ $NVR_MODE == 0 ];then
	insmod hdal/kflow_common/ms/ms.ko max_channels=8
	insmod hdal/kflow_common/em/em.ko max_channels=8
else
	insmod hdal/kflow_common/ms/ms.ko 
	insmod hdal/kflow_common/em/em.ko 
fi
insmod hdal/kdrv_videoout/lcd_codec/lcd_codec.ko
insmod hdal/kdrv_videoout/tve100/nvt_tve100.ko hotplug_enable=0
insmod hdal/kdrv_videoout/hdmi/nvt_hdmi20.ko hdmi_vid144_support=0
insmod hdal/kdrv_videoout/lcd310/flcd300-common.ko 
insmod hdal/kdrv_videoout/lcd310/flcd300-pip.ko suspend_state=1 
insmod hdal/kdrv_videoout/lcd210/flcd200-common.ko
insmod hdal/kdrv_videoout/lcd210/flcd200-pip.ko suspend_state=1

insmod extdrv/at24c.ko
insmod extdrv/fvideo.ko
insmod extdrv/rtc_BM.ko
insmod hdal/kflow_videocapture/vcap316/vcap316_common.ko
insmod hdal/kflow_videocapture/vcap316/vcap316_host0.ko

insmod hdal/kdrv_videoprocess/vpe321/vpe321_drv.ko
if [ $NVR_MODE == 0 ];then
	insmod hdal/kflow_videoprocess/vpe/kflow_vpe.ko mod_init=1 max_chip_num=1 max_eng_num=1 max_minor_num=40 max_md_lv_num=8 max_total_cam_ch=32
else
	insmod hdal/kflow_videoprocess/vpe/kflow_vpe.ko mod_init=1 max_chip_num=1 max_eng_num=1 max_minor_num=255 max_md_lv_num=0 max_total_cam_ch=32
fi
insmod hdal/kdrv_audioio/nvt_audio.ko
insmod hdal/kflow_audioio/kflow_audio.ko

insmod hdal/kdrv_videoprocess/dei321/dei321_drv.ko
insmod hdal/kflow_videoprocess/di/kflow_di.ko max_chip_num=1 max_eng_num=1 max_minor_num=64

if [ $NVR_MODE == 0 ];then
	insmod hdal/kdrv_videojpeg/kdrv_jpg.ko
	insmod hdal/kdrv_videoenc/h26xenc.ko h26x_enc_max_width=1920 h26x_enc_max_height=1920 h26x_enc_max_chn=24 max_total_cam_ch=24
	insmod hdal/nvt_vencrc/nvt_vencrc.ko
	insmod hdal/kflow_videoenc/kflow_videoenc.ko
	insmod hdal/kdrv_videodec/h26xdec.ko h26xd_max_width=1920 h26xd_max_height=1920 max_total_cam_ch=8 h265_max_support_vps=2 h265_max_support_sps=2 h264_max_support_sps=2

	insmod hdal/kflow_videodec/kflow_videodec.ko
else
	insmod hdal/kdrv_videojpeg/kdrv_jpg.ko
	insmod hdal/kdrv_videoenc/h26xenc.ko h26x_enc_max_width=1920 h26x_enc_max_height=1920 h26x_enc_max_chn=24 max_total_cam_ch=24
	insmod hdal/nvt_vencrc/nvt_vencrc.ko
	insmod hdal/kflow_videoenc/kflow_videoenc.ko
	insmod hdal/kdrv_videodec/h26xdec.ko h26xd_max_width=2888 h26xd_max_height=2048 max_total_cam_ch=32 h265_max_support_vps=2 h265_max_support_sps=2 h264_max_support_sps=2

	insmod hdal/kflow_videodec/kflow_videodec.ko
fi
insmod hdal/kdrv_videoosg/kdrv_osg.ko
insmod hdal/kflow_videoosg/kflow_osg.ko max_pattern_width=1280 max_pattern_height=64 max_minor_num=64 max_win=128

insmod extdrv/mtprealloc7601Usta.ko
insmod extdrv/mt7601Usta.ko

#insmod extdrv/rtutil3070sta.ko
#insmod extdrv/rt3070sta.ko
#insmod extdrv/rtnet3070sta.ko

insmod hdal/kdrv_gfx2d/gm2d.ko
if [ $NVR_MODE == 0 ];then
	insmod hdal/kflow_common/gs/gs.ko max_channels=8
else
	insmod hdal/kflow_common/gs/gs.ko
fi
insmod hdal/kflow_common/usr/usr_proc.ko
if [ $NVR_MODE == 0 ];then
	insmod hdal/kflow_common/vpd/vpd.ko quiet=1 max_channels=8
else
	insmod hdal/kflow_common/vpd/vpd.ko quiet=1 max_channels=16
fi
insmod hdal/comm/ddr_arb/ddr_arb.ko
insmod hdal/comm/irdet/kdrv_irdet.ko
echo i2c 0x11011 > /proc/nvt_info/nvt_pinmux/pinmux_set


echo doing mdev-s
mdev -s
echo done!!

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
#devmem 0xfad00044 32 0x40002F0F
#echo 3 > /sys/module/nvt_tve100/parameters/cvbs_comp_out_ref
echo w pri 5 0 > /proc/nvt_drv_sys/dram_info  # Set DDR CPU(ch5) high priority
echo w dynamic_pri 1 2 > /proc/nvt_drv_sys/dram_info  # Set DDR LCD(ch1) dynamic high priority
echo w dynamic_pri 7 2 > /proc/nvt_drv_sys/dram_info  # Set DDR CAP(ch7) dynamic high priority

echo w grant 0 2 > /proc/nvt_drv_sys/dram_info  # set DDR JPG,OSG,VPE316(ch0) grant 2
echo w grant 1 2 > /proc/nvt_drv_sys/dram_info  # set DDR LCD(ch1) grant 2
echo w grant 2 4 > /proc/nvt_drv_sys/dram_info  # set DDR VDEC(ch2) grant 4
echo w grant 3 2 > /proc/nvt_drv_sys/dram_info  # set DDR VPE536(ch3) grant 2
echo w grant 4 2 > /proc/nvt_drv_sys/dram_info  # set DDR CNN,NUE(ch4) grant 2
echo w grant 5 2 > /proc/nvt_drv_sys/dram_info  # set DDR CPU,DSP(ch5) grant 2
echo w grant 6 8 > /proc/nvt_drv_sys/dram_info  # set DDR VENC(ch6) grant 8
echo w grant 7 2 > /proc/nvt_drv_sys/dram_info  # set DDR VCAP(ch7) grant 2
#echo 1 > /proc/videograph/gs/dbglevel
#echo 1 > /proc/videograph/lcd300/dbglevel

echo 1 4 0 0 3 58 > /proc/vcap316/vcap0/scaler_rule
echo 2 4 0 0 3 58 > /proc/vcap316/vcap0/scaler_rule 
echo 3 4 0 0 7 50 > /proc/vcap316/vcap0/scaler_rule
echo 4 4 0 0 15 34 > /proc/vcap316/vcap0/scaler_rule

while true
do
	echo 3 > /proc/sys/vm/drop_caches 
	sleep 3
done &

echo SwToDelay 60 > /proc/kdrv_h26xd/param

#echo 0 > /proc/ssca/gating_en


