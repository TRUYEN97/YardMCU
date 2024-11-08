#!/bin/bash

cd /home/admin/Desktop/YardSocket || { echo "khong the chuyen den thu muc"; exit 1; }

python3 yardSocket.py

if [ $? -ne 0 ]; then
    echo "khong the khoi dong SOCKET"
    exit 1
fi

exit 0