#boot_ver=v1.5
#chipver=`head -1 /proc/pmu/chipver`
#chipid=`echo $chipver | cut -c 1-4`
echo -----------------------------------------------------------
echo "        Boot DVR_4CH"
echo -----------------------------------------------------------
echo "/sbin/mdev" > /proc/sys/kernel/hotplug

MODEL_PATH=$(dirname $MODEL)
MODEL_NAME="${MODEL_PATH##*/}"
AD_MODULE="techpoint"
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

if [ "$MODEL_NAME" == "cfg_98321_DDR3_4GX1_NAND_DVR_SYS" ]; then
    # [NT98321 SYS] 4CH, camera support to 960H, 720P@30, 1080P@30, 4M@15, 5M@10
    modprobe tp28xx_kdrv drv_mode=0 dev_num=1 ibus=1 iaddr=0x88 vout_mode=3 video_mux=1 vout_xcap=0x01020000 vout_vi=0x01020000 vout_cdly=0x08080000 clk_dly=0x00000000 clk_inv=0x01010000 ch_map=2 clk_used=0x1 rstb_used=106
    modprobe vcap316_common
    modprobe vcap316_host0

elif [ "$MODEL_NAME" == "cfg_98323_4GX2_NAND_EVB" ]; then
    # set pinumux for tp28xx on EVB  
    echo i2c 0x10001 > /proc/nvt_info/nvt_pinmux/pinmux_set
    # [NT98323 EVB] 4CH, camera support to 960H, 720P@30, 1080P@30, 4M@30 , 5M@20, 8M@15
	if [ "$AD_MODULE" == "techpoint" ]; then	
        modprobe tp28xx_kdrv drv_mode=0 dev_num=1 ibus=0 iaddr=0x88 vout_mode=5 video_mux=1 vout_xcap=0x01020304 vout_vi=0x01020304 clk_dly=0x00000000 clk_inv=0x01010101 ch_map=0 clk_used=0x1 rstb_used=106
	else	
		modprobe nvp6158_kdrv drv_mode=0 dev_num=2 ibus=0,0 iaddr=0x60,0x62 vout_mode=0,0 vout_xcap=0x0201,0x0403 vout_vi=0x0201,0x0403 vout_cdly=0x0606,0x0606 clk_dly=0x0000,0x0000 clk_inv=0x0000,0x0000 ch_map=11 clk_used=0x1 rstb_used=106 clk_driving=2
	fi	
    modprobe vcap316_common
    modprobe vcap316_host0

elif [ "$MODEL_NAME" == "cfg_98321_DDR3_4GX1_NAND_EVB" ]; then
    # set pinumux for tp28xx on EVB  
    echo i2c 0x10001 > /proc/nvt_info/nvt_pinmux/pinmux_set
    # [NT98321 EVB] 4CH, camera support to 960H, 720P@30, 1080P@30, 4M@15, 5M@10
    modprobe tp28xx_kdrv drv_mode=0 dev_num=1 ibus=0 iaddr=0x88 vout_mode=3 video_mux=1 vout_xcap=0x01000002 vout_vi=0x01000002 clk_dly=0x00000000 clk_inv=0x01000001 ch_map=17 clk_used=0x1 rstb_used=106
    modprobe vcap316_common
    modprobe vcap316_host0

else
    echo 105 > /sys/class/gpio/export
    gpio105=$(cat /sys/class/gpio/gpio105/value)
    pkgid=98321
    
    if [ "$gpio105" == "1" ] ; then
        echo This is 98321 System Board.
        # [NT98321 SYS] 4CH, camera support to 960H, 720P@30, 1080P@30, 4M@15, 5M@10
        modprobe tp28xx_kdrv drv_mode=0 dev_num=1 ibus=1 iaddr=0x88 vout_mode=3 video_mux=1 vout_xcap=0x01020000 vout_vi=0x01020000 clk_dly=0x00000000 clk_inv=0x01010000 ch_map=2 clk_used=0x1 rstb_used=106
        modprobe vcap316_common
        modprobe vcap316_host0
    else
        echo This is 98321 EVB.
        # set pinumux for tp28xx on EVB  
        echo i2c 0x10001 > /proc/nvt_info/nvt_pinmux/pinmux_set
        # [NT98321 EVB] 4CH, camera support to 960H, 720P@30, 1080P@30, 4M@15, 5M@10
        modprobe tp28xx_kdrv drv_mode=0 dev_num=1 ibus=0 iaddr=0x88 vout_mode=3 video_mux=1 vout_xcap=0x01000002 vout_vi=0x01000002 clk_dly=0x00000000 clk_inv=0x01000001 ch_map=17 clk_used=0x1 rstb_used=106
        modprobe vcap316_common
        modprobe vcap316_host0
    fi
fi

modprobe vpe321_drv
modprobe kflow_vpe mod_init=1 max_chip_num=1 max_eng_num=1 max_minor_num=255 max_md_lv_num=16 max_total_cam_ch=21

modprobe nvt_audio
modprobe kflow_audio

modprobe dei321_drv
modprobe kflow_di max_chip_num=1 max_eng_num=1 max_minor_num=32 max_total_cam_ch=20

modprobe kdrv_jpg
modprobe h26xenc h26x_enc_max_width=2560 h26x_enc_max_height=1440 max_total_cam_ch=21
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
if [ "$AD_MODULE" == "techpoint" ]; then
    /mnt/mtd/module_init /mnt/mtd/cfg_DVR_4CH.dtb 0 1 1 1 0 &
else
    /mnt/mtd/module_init /mnt/mtd/cfg_DVR_4CH.dtb 0 2 1 1 0 &
fi

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
echo w dynamic_pri 7 2 > /proc/nvt_drv_sys/dram_info  # Set DDR CAP(ch7) dynamic high priority

echo w grant 0 2 > /proc/nvt_drv_sys/dram_info  # set DDR JPG,OSG,VPE316(ch0) grant 2
echo w grant 1 2 > /proc/nvt_drv_sys/dram_info  # set DDR LCD(ch1) grant 2
echo w grant 2 4 > /proc/nvt_drv_sys/dram_info  # set DDR VDEC(ch2) grant 4
echo w grant 3 2 > /proc/nvt_drv_sys/dram_info  # set DDR VPE536(ch3) grant 2
echo w grant 4 2 > /proc/nvt_drv_sys/dram_info  # set DDR CNN,NUE(ch4) grant 2
echo w grant 5 2 > /proc/nvt_drv_sys/dram_info  # set DDR CPU,DSP(ch5) grant 2
echo w grant 6 8 > /proc/nvt_drv_sys/dram_info  # set DDR VENC(ch6) grant 8
echo w grant 7 2 > /proc/nvt_drv_sys/dram_info  # set DDR VCAP(ch7) grant 2
