#boot_ver=v1.5
#chipver=`head -1 /proc/pmu/chipver`
#chipid=`echo $chipver | cut -c 1-4`
echo -----------------------------------------------------------
echo "        Boot DVR_FPGA"
echo -----------------------------------------------------------
echo "/sbin/mdev" > /proc/sys/kernel/hotplug
mem w 0xfe000050 0x03F18043
mem w 0xfe000058 0x05400103
mem w 0xfe000060 0x383C7A00 
mem w 0xfe000070 0xFF9FE8FC 
mem w 0xfe000078 0xFFFCE17F
mem w 0xfe000070 0xFF9FE8FC 

modprobe nvt_dmasys
#cat /proc/frammap/ddr_info
modprobe log log_ksize=4096 crash_notify=/mnt/mtd/crash.sh	#crash to execute /mnt/mtd/crash.sh
echo /tmp > /proc/videograph/dumplog		#change log path to /tmp

#modprobe ddr_monitor
modprobe ms max_channels=4
modprobe em max_channels=4
modprobe lcd_codec
modprobe nvt_tve100
modprobe flcd300-common
modprobe flcd300-pip suspend_state=1 
modprobe flcd200-common
modprobe flcd200-pip suspend_state=1
modprobe nvt_hdmi20
#modprobe vcap316_common
#modprobe vcap316_host0 sys_mode=1

#socket board
#modprobe tp28xx_kdrv drv_mode=0 dev_num=1 ibus=0 iaddr=0x88 vout_mode=5 vout_xcap=0x01020304 vout_vi=0x01020304 ch_map=0 clk_used=0x4 rstb_used=91

##system board (2MP)
#if [ "$chipid" = "8312" ] 
#then
#modprobe tp28xx_kdrv drv_mode=0 dev_num=2 ibus=0,0 iaddr=0x88,0x8a vout_mode=3,3 video_mux=1 vout_xcap=0x01020000,0x03040000 vout_vi=0x01020000,0x03040000 #vout_cdly=0x0d0d0000,0x0b0b0000 clk_inv=0x01010000,0x01010000 clk_dly=0x01010000,0x01010000 ch_map=2 clk_used=0x2 clk_driving=3 rstb_used=90
#else
#modprobe tp28xx_kdrv drv_mode=0 dev_num=2 ibus=0,0 iaddr=0x88,0x8a vout_mode=3,3 vout_xcap=0x01020000,0x03040000 vout_vi=0x01020000,0x03040000 ch_map=2 clk_used=0x20 rstb_used=1
#fi

#system board (one channel: 5MP/8MP)
#modprobe tp28xx_kdrv drv_mode=0 dev_num=2 ibus=0,0 iaddr=0x88,0x8a vout_mode=5,5 vout_xcap=0x01020000,0x03040000 vout_vi=0x01020000,0x03040000 ch_map=12 clk_used=0x20 rstb_used=1

modprobe vpe321_drv
modprobe kflow_vpe mod_init=1 max_chip_num=1 max_eng_num=1 max_minor_num=255 max_md_lv_num=16 max_total_cam_ch=32
#modprobe vpe536 mod_init=1 max_chip_num=1 max_eng_num=1 max_minor_num=255 max_dce_2d_num=0 max_md_lv_num=16 max_total_cam_ch=32
#modprobe vpe316 max_chip_num=1 max_eng_num=1 max_minor_num=255 max_total_cam_ch=10
modprobe nvt_audio
modprobe kflow_audio
#modprobe adda301
#modprobe h26xenc h26x_enc_max_width=2560 h26x_enc_max_height=1440 max_minor_num=64 max_total_cam_ch=10
#modprobe decoder
#modprobe hevc_dec h265d_max_width=2560 h265d_max_height=1440 h265d_max_chn=8 max_total_cam_ch=10
#modprobe favc_dec h264d_max_width=2560 h264d_max_height=1440 h264d_max_chn=8 max_total_cam_ch=10
#modprobe fmcp_drv
#modprobe fmjpeg_drv
modprobe kdrv_osg
modprobe kflow_osg
#modprobe gm2d
modprobe gs max_channels=4
modprobe usr_proc
modprobe vpd quiet=0 max_channels=4
modprobe h26xdec h26xd_max_width=2560 h26xd_max_height=1440 max_total_cam_ch=10
modprobe kdrv_jpg
modprobe kflow_videodec
mdev -s

#devmem 0xfc400034 32 0x06000300  # DDR Grant--> CH3:VPE536 CH2:H265D CH1:LCD310    CH0:H264/JPEG/VPE316/OSG
#devmem 0xfc400038 32 0x02060000  # DDR Grant--> CH7:VCAP   CH6:H265E CH5:CPU/CEVA  CH4:CNN

modprobe vcap316_common vi_dup=0
modprobe vcap316_host0  sys_mode=0 vi_mode=1,0,1,0 cap_md=1,128,128,1
modprobe vcap316_generic_m0 vi=0 vi_src=0 ch_norm=5 vch_id=0    
modprobe vcap316_generic_m1 vi=2 vi_src=0 ch_norm=5 vch_id=1   
#devmem 0xfe000024                # Read to check LCD310 hi priority (bit 5)
#devmem 0xfda00744 32 0x80000000  # LCD310 DDR request
#devmem 0xfda00748 32 0x00800020  # LCD310 dma threshold
#devmem 0xfda0074C 32 0x00800020  # LCD310 dma threshold
#devmem 0xfda00750 32 0x00800020  # LCD310 dma threshold
#devmem 0xfda00754 32 0x00800020  # LCD310 dma threshold
#devmem 0xfda00758 32 0x00800020  # LCD310 dma threshold
#devmem 0xfda0075c 32 0x00800020  # LCD310 dma threshold

# Syntax: module_init  [dtb_file] [is_dump_dts] [is_init_videocap] [is_init_videoout] [is_init_audio]
#/mnt/sd/module_init cfg_DVR_FPGA.dtb &
/mnt/sd/module_init cfg_DVR_FPGA.dtb 0 0 1 1 &


#echo 22 1 > /proc/vcap316/vcap0/vcap0.0/cfg/global # BIT[0]:DDR0 BIT[1]:DDR1
#echo 21 1 > /proc/vcap316/vcap0/vcap0.0/cfg/global	# BIT[0]:DDR0 BIT[1]:DDR1
