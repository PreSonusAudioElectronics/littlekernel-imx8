#! /bin/bash

echo 'Shutdown virtual ethernet adapter'
rmmod virtual_eth_adapter

echo 'Shutdown lk cell'
jailhouse cell shutdown lk

echo 'Destroy lk cell'
jailhouse cell destroy lk

echo 'Disabling Jailhouse root cell'
jailhouse disable

echo 'Rmmod jailhouse.ko'
rmmod jailhouse.ko
