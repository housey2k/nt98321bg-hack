#boot_ver=v1.5
#chipver=`head -1 /proc/pmu/chipver`
#chipid=`echo $chipver | cut -c 1-4`
echo -----------------------------------------------------------
echo "        Boot DVR_FPGA"
echo -----------------------------------------------------------
echo "/sbin/mdev" > /proc/sys/kernel/hotplug


modprobe nvt_dmasys
modprobe log log_ksize=4096 crash_notify=/mnt/mtd/crash.sh	#crash to execute /mnt/mtd/crash.sh
echo /tmp > /proc/videograph/dumplog		#change log path to /tmp

modprobe ms max_channels=4
modprobe em max_channels=4

modprobe lcd_codec
modprobe nvt_tve100
modprobe flcd300-common
modprobe flcd300-pip suspend_state=1 
modprobe flcd200-common
modprobe flcd200-pip suspend_state=1
modprobe nvt_hdmi20

modprobe vcap316_common vi_dup=0
modprobe vcap316_host0  sys_mode=0 vi_mode=1,0,0,1 cap_md=1,128,128,1
modprobe vcap316_generic_m0 vi=0 vi_src=0 ch_norm=5 vch_id=0    
modprobe vcap316_generic_m1 vi=3 vi_src=3 ch_norm=1 vch_id=1   

modprobe vpe321_drv
modprobe kflow_vpe mod_init=1 max_chip_num=1 max_eng_num=1 max_minor_num=255 max_md_lv_num=16 max_total_cam_ch=32

modprobe nvt_audio
modprobe kflow_audio

modprobe dei321_drv
modprobe kflow_di

modprobe kdrv_jpg
modprobe h26xenc h26x_enc_max_width=2560 h26x_enc_max_height=1440 max_minor_num=64 max_total_cam_ch=10
modprobe nvt_vencrc
modprobe kflow_videoenc
modprobe h26xdec h26xd_max_width=2560 h26xd_max_height=1440 max_total_cam_ch=10
modprobe kflow_videodec

modprobe kdrv_osg
modprobe kflow_osg
#modprobe gm2d
modprobe gs max_channels=4
modprobe usr_proc
modprobe vpd quiet=0 max_channels=4

echo doing mdev-s
mdev -s
echo done!!

# Syntax: module_init  [dtb_file] [is_dump_dts] [is_init_videocap] [is_init_videoout] [is_init_audio]
#/mnt/sd/module_init cfg_DVR_FPGA.dtb &
/mnt/sd/module_init cfg_DVR_FPGA.dtb 0 0 1 1 &
