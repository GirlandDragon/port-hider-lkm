#!/system/bin/sh
MODDIR=${0%/*}
KOPATH=$MODDIR/lkm/port_hider.ko
CONFIG=/data/adb/MaskMyScene/config.txt
PROP=$MODDIR/module.prop

update_desc() {
    sed -i "s|^description=.*|description=$1|" "$PROP"
}

update_desc "正在启动中...（若持续显示则加载失败）"

if [ -f "$CONFIG" ]; then
    PARAMS=$(grep -v '^#' "$CONFIG" | grep -v '^$' | tr '\n' ' ')
    err=$(insmod "$KOPATH" $PARAMS 2>&1)
else
    update_desc "错误：未找到配置文件 $CONFIG"
    exit 0
fi

case "$err" in
    *"File exists"*) update_desc "运行中" ; exit 0 ;;
    "")             update_desc "运行中" ;;
    *)              update_desc "错误：$(echo "$err" | head -1 | tr '\n' ' ')" ;;
esac
