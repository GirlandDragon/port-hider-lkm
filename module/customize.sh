#!/system/bin/sh
SKIPUNZIP=1
# KernelSU sets $MODPATH before sourcing this script.
# Do NOT redefine it: '${0%/*}' is wrong when sourced.
CONFIG_DIR=/data/adb/MaskMyScene

KMI=$(uname -r | cut -d. -f1-2)
AV=$(echo "$(uname -r)" | grep -oE 'android[0-9]+' | head -1)

ui_print "内核：$(uname -r)"
ui_print "KMI：${KMI}  标识：${AV:-（未检测到）}"

if [ -z "$AV" ]; then
    ui_print "错误：内核版本中未检测到 Android 版本标识"
    exit 1
fi

# Extract whitelisted files only — anything else in ZIP is discarded
unzip -o "$ZIPFILE" module.prop service.sh action.sh uninstall.sh -d "$MODPATH" 2>/dev/null
unzip -o "$ZIPFILE" "lkm/port_hider-${AV}-${KMI}.ko" -d "$MODPATH" 2>/dev/null
mv "$MODPATH/lkm/port_hider-${AV}-${KMI}.ko" "$MODPATH/lkm/port_hider.ko" 2>/dev/null

if [ -f "$MODPATH/lkm/port_hider.ko" ]; then
    ui_print "匹配 ${AV}-${KMI} → lkm/port_hider.ko"
else
    ui_print "没有匹配 ${AV}-${KMI} 的内核模块"
    exit 1
fi

# Default config
mkdir -p "$CONFIG_DIR"
if [ ! -f "$CONFIG_DIR/config.txt" ]; then
    ui_print "创建默认配置：$CONFIG_DIR/config.txt"
    cat > "$CONFIG_DIR/config.txt" <<EOF
# MaskMyScene configuration
ports=8765,8788,14731,14754
whitelist_uids=0,1000,2000
hooks=127
MinPath=2
EOF
fi

set_perm_recursive $MODPATH 0 0 0755 0644
set_perm_recursive "$CONFIG_DIR" 0 0 0755 0644

# ── Scene 白名单 ──────────────────────────────────────────

SCENE_PKG=com.omarea.vtools
SCENE_UID=$(dumpsys package "$SCENE_PKG" 2>/dev/null | grep userId | sed 's/.*userId=//')
[ -z "$SCENE_UID" ] && SCENE_UID=$(stat -c "%u" "/data/data/$SCENE_PKG" 2>/dev/null)

if [ -z "$SCENE_UID" ]; then
    ui_print ""
    ui_print "警告：未检测到 $SCENE_PKG"
    ui_print "如未安装此应用，加载本模块无任何意义。"
    ui_print ""
elif grep -q "whitelist_uids=.*$SCENE_UID" "$CONFIG_DIR/config.txt" 2>/dev/null; then
    ui_print "UID $SCENE_UID（$SCENE_PKG）已在白名单配置中"
else
    ui_print ""
    ui_print "检测到 $SCENE_PKG（UID: $SCENE_UID）"
    ui_print "将其加入白名单？否则该应用会被本模块拦截导致无法连接其守护进程。"
    ui_print "音量上 = 是    音量下 = 否"
    ui_print ""

    V=$(mktemp)
    getevent -l 2>/dev/null | while read l; do
        case "$l" in
            *KEY_VOLUMEUP*)   echo 1 > "$V"; break ;;
            *KEY_VOLUMEDOWN*) echo 2 > "$V"; break ;;
        esac
    done &
    PID=$!
    wait "$PID" 2>/dev/null
    CHOICE=$(cat "$V" 2>/dev/null)
    rm -f "$V"

    if [ "$CHOICE" = "1" ]; then
        sed -i "s/^whitelist_uids=\(.*\)/whitelist_uids=\1,$SCENE_UID/" "$CONFIG_DIR/config.txt"
        ui_print "已添加 UID $SCENE_UID 到白名单"
    else
        ui_print "跳过，如有需要可手动编辑 $CONFIG_DIR/config.txt"
    fi
fi
