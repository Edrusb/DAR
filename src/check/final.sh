#!/bin/sh

echo "+-------------------------------+"
echo "| ALL TESTS PASSED SUCCESSFULLY |"
echo "+-------------------------------+"

if [ -x ~denis/Scripts/send_sms ] ; then
    ~denis/Scripts/send_sms "make check successful !"
fi
