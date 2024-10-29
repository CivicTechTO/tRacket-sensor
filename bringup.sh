#!/bin/bash
#
# Run this script with a newly manufactured tRacket board connected to give it
# a unique storage encryption key and flash the latest firmware.
# The "tail" command will show the board's UUID; this needs to be added to the
# server for device registration.

dd if=/dev/urandom of=hmac_key bs=1 count=32
echo "BURN" | pio pkg exec -- espefuse.py --port /dev/ttyACM0 burn_key BLOCK4 hmac_key HMAC_UP
rm hmac_key
pio run -t upload
tail -F /dev/ttyACM0

