#!/bin/bash

PRODUCT=NVR_8CH
echo To create $PRODUCT product JFFS2 image for /mnt/mtd.
echo ""

day=$(date +%F)

NVT_TOOLS=$BUILD_DIR/nvt-tools
ROOTFS=$SDK_PATH/BSP/root-fs/rootfs
SAMPLE_SRC=$NVT_HDAL_DIR/samples
PRODUCT_SRC=$SAMPLE_SRC/hdal_product/$PRODUCT
rm -rf $PRODUCT_SRC/image;mkdir $PRODUCT_SRC/image
cp $PRODUCT_SRC/vg_boot.sh $PWD/image/
cp $PRODUCT_SRC/crash.sh $PWD/image/
cp $SAMPLE_SRC/hdal_product/module_init $PWD/image
cp $PRODUCT_SRC/cfg_${PRODUCT}.dtb $PWD/image
cp $PRODUCT_SRC/nvr_perf $PWD/image
cp $PRODUCT_SRC/display_change_to_1080P.sh $PWD/image
cp $SAMPLE_SRC/pattern/1920x1080_ref2* $PWD/image
cp $SAMPLE_SRC/pattern/400x100_argb1555.bin $PWD/image
cp $SAMPLE_SRC/pattern/logo_720x576.jpg $PWD/image
mkdir $PRODUCT_SRC/image/00_$day-$PRODUCT
chmod 755 $PRODUCT_SRC/image/*

if [ -e $PRODUCT_SRC/$PRODUCT.nand.ubifs.bin ] ; then
    rm -f $PRODUCT_SRC/$PRODUCT.nand.ubifs.bin
fi
if [ -e $PRODUCT_SRC/$PRODUCT.nor.jffs2.bin ] ; then
    rm -f $PRODUCT_SRC/$PRODUCT.nor.jffs2.bin
fi
if [ -e $PRODUCT_SRC/$PRODUCT.nand.ubifs.raw ] ; then
    rm -f $PRODUCT_SRC/$PRODUCT.nand.ubifs.raw
fi
if [ -e $PRODUCT_SRC/$PRODUCT.nor.jffs2.raw ] ; then
    rm -f $PRODUCT_SRC/$PRODUCT.nor.jffs2.raw
fi

# Read config for "mkfs.jffs2 & mkfs.ubifs" tool from rootfs project 
ROOTFS_MTD_CFG_FILE=${CONFIG_DIR}/cfg_gen/mtd_cfg.txt
if [ ! -f $ROOTFS_MTD_CFG_FILE ]; then
    ROOTFS_MTD_CFG_FILE=$ROOTFS_DIR/mtd_cfg.txt
fi
echo Use config file: $ROOTFS_MTD_CFG_FILE

while IFS= read -r line
do
    export $(echo $line | head -n1 | cut -d " " -f1) > /dev/null 2>&1
done < $ROOTFS_MTD_CFG_FILE
ROOTFS_JFFS2_BLK_SIZE_VAL=$(echo $ROOTFS_JFFS2_BLK_SIZE| cut -d \" -f2)
ROOTFS_UBI_BLK_SIZE_VAL=$(echo $ROOTFS_UBI_BLK_SIZE| cut -d \" -f2)
ROOTFS_JFFS2_COMPRESS_MODE_VAL=$(echo $ROOTFS_JFFS2_COMPRESS_MODE| cut -d \" -f2)
ROOTFS_UBI_COMPRESS_MODE_VAL=$(echo $ROOTFS_UBI_COMPRESS_MODE| cut -d \" -f2)

THIS_MODEL_NAME=$(echo ${NVT_PRJCFG_MODEL_CFG} | awk -F'/' '{print $(NF-1)}')
if echo "$THIS_MODEL_NAME" | grep -q "NOR"; then
    ROOTFS_JFFS2_APP_SIZE_VAL=$(echo $ROOTFS_JFFS2_APP_NOR_SIZE| cut -d \" -f2)
else
    ROOTFS_JFFS2_APP_SIZE_VAL=$(echo $ROOTFS_JFFS2_APP_SIZE| cut -d \" -f2)
fi

echo Make JFFS2: mkfs.jffs2 -n -e $ROOTFS_JFFS2_BLK_SIZE_VAL \
           -d $PRODUCT_SRC/image/ -o $PRODUCT_SRC/$PRODUCT.nor.jffs2.raw
mkfs.jffs2 -n -e $ROOTFS_JFFS2_BLK_SIZE_VAL \
           -d $PRODUCT_SRC/image/ -o $PRODUCT_SRC/$PRODUCT.nor.jffs2.raw

$NVT_TOOLS/nvt-ld-op --packsum-src=$PRODUCT_SRC/$PRODUCT.nor.jffs2.raw --packsum-dst=$PRODUCT_SRC/$PRODUCT.nor.jffs2.bin --packsum-type=0x9 
echo create $PRODUCT_SRC/$PRODUCT.nor.jffs2.bin done

echo "[ubifs]" > ubi.cfg
echo "mode=ubi" >> ubi.cfg
echo "image=$PRODUCT_SRC/ubifs.bin" >> ubi.cfg
echo "vol_id=0" >> ubi.cfg
echo "vol_size=$(echo $ROOTFS_UBI_APP_MAX_LEB_COUNT\*$ROOTFS_UBI_ERASE_BLK_SIZE|bc)" >> ubi.cfg
echo "vol_type=dynamic" >> ubi.cfg
echo "vol_name=app" >> ubi.cfg
echo Make UBIFS: mkfs.ubifs --squash-uids -x $ROOTFS_UBI_COMPRESS_MODE_VAL -r $PRODUCT_SRC/image -o $PRODUCT_SRC/ubifs.bin \
            -m $ROOTFS_UBI_PAGE_SIZE -e $ROOTFS_UBI_ERASE_BLK_SIZE -c $ROOTFS_UBI_APP_MAX_LEB_COUNT -v
mkfs.ubifs --squash-uids -x $ROOTFS_UBI_COMPRESS_MODE_VAL -r $PRODUCT_SRC/image -o $PRODUCT_SRC/ubifs.bin \
            -m $ROOTFS_UBI_PAGE_SIZE -e $ROOTFS_UBI_ERASE_BLK_SIZE -c $ROOTFS_UBI_APP_MAX_LEB_COUNT -v
ubinize -o $PRODUCT_SRC/$PRODUCT.nand.ubifs.raw -m $ROOTFS_UBI_PAGE_SIZE -p $ROOTFS_UBI_BLK_SIZE_VAL -s $ROOTFS_UBI_PAGE_SIZE ubi.cfg -v
$NVT_TOOLS/nvt-ld-op --packsum-src=$PRODUCT_SRC/$PRODUCT.nand.ubifs.raw --packsum-dst=$PRODUCT_SRC/$PRODUCT.nand.ubifs.bin --packsum-type=0x9 

rm -f ubi.cfg 2> /dev/null
rm -f ubifs.bin 2> /dev/null
echo create $PRODUCT_SRC/$PRODUCT.nand.ubifs.bin done

echo $OUTPUT_DIR
if [ -d $OUTPUT_DIR ] ; then
    echo copy product image to $OUTPUT_DIR done
    cp $PRODUCT_SRC/$PRODUCT.nand.ubifs.bin $OUTPUT_DIR
    cp $PRODUCT_SRC/$PRODUCT.nor.jffs2.bin $OUTPUT_DIR
    cp $PRODUCT_SRC/$PRODUCT.nand.ubifs.raw $OUTPUT_DIR/raw
    cp $PRODUCT_SRC/$PRODUCT.nor.jffs2.raw $OUTPUT_DIR/raw
fi
