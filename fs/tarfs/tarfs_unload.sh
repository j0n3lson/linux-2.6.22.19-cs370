#!/bin/sh
#adapted from Linux Device Drivers 3 & 2
module="tarfs"
device="tarfs"
chardevice="tarfsr"

# invoke rmmod with all arguments we got
/sbin/rmmod $module $* || exit 1

# Remove stale nodes

rm -f /dev/${device}[0-3] /dev/${device}
rm -f /dev/${chardevice}[0-3] /dev/${chardevice}





