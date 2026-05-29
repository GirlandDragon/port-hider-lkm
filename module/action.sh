#!/system/bin/sh
MODDIR=${0%/*}
CONFIG=/data/adb/MaskMyScene/config.txt
PROP=$MODDIR/module.prop

update_desc() {
    sed -i "s|^description=.*|description=$1|" "$PROP"
}

if [ ! -f "$CONFIG" ]; then
    update_desc "错误：未找到配置文件 $CONFIG"
    exit 0
fi

PORTS=$(grep "^ports=" "$CONFIG" | head -1 | cut -d= -f2)
[ -z "$PORTS" ] && PORTS="8765,8788,14731,14754"

# Extract whitelist UIDs from config
WLUIDS=$(grep "^whitelist_uids=" "$CONFIG" | head -1 | cut -d= -f2)

# Find a non-whitelisted app UID for testing
# (root/0 sees everything, test must run as a real app)
TEST_UID=
for d in /data/data/*/; do
    u=$(stat -c "%u" "$d" 2>/dev/null)
    [ -z "$u" ] && continue
    echo ",$WLUIDS," | grep -q ",$u," && continue
    TEST_UID=$u
    break
done

if [ -z "$TEST_UID" ]; then
    update_desc "错误：未找到可用于检测的非白名单应用 UID"
    exit 0
fi

open=0
total=0
OLD_IFS=$IFS; IFS=,
for p in $PORTS; do
    total=$((total + 1))
    su "$TEST_UID" sh -c "timeout 1 nc 127.0.0.1 $p </dev/null" 2>/dev/null
    [ $? -eq 0 ] && open=$((open + 1))
done
IFS=$OLD_IFS

if [ "$open" -eq 0 ]; then
    update_desc "完全隐藏（未检测到任何端口）"
else
    update_desc "部分隐藏（${open}/${total} 个端口可访问）"
fi
