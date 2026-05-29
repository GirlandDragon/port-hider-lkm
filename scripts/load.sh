#!/bin/sh

MODULE=lkm/port_hider.ko
MODNAME=port_hider

case "$1" in
    insmod)
        PORTS="${2:-8765,8788,14731,14754}"
        PARAMS="ports=$PORTS"
        echo "Loading $MODNAME: $PARAMS"
        su -c "insmod $MODULE $PARAMS" && echo "OK" || echo "FAIL"
        ;;
    rmmod)
        echo "Unloading $MODNAME"
        su -c "rmmod $MODNAME" && echo "OK" || echo "FAIL"
        ;;
    status)
        if su -c "lsmod 2>/dev/null | grep -q $MODNAME"; then
            echo "$MODNAME: loaded"
            su -c "dmesg | grep port_hider | tail -5"
        else
            echo "$MODNAME: not loaded"
        fi
        ;;
    *)
        echo "Usage: $0 {insmod|rmmod|status} [ports]"
        echo ""
        echo "Examples:"
        echo "  $0 insmod                         # default ports"
        echo "  $0 insmod 8765,8788               # custom ports"
        echo "  $0 rmmod"
        echo "  $0 status"
        exit 1
        ;;
esac
