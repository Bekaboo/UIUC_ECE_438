#!/usr/bin/env bash
# vim: ft=sh ts=4 sw=4 sts=4 et :

sudo tc qdisc del dev enp0s3 root 2>/dev/null
sudo tc qdisc add dev enp0s3 root handle 1:0 netem delay 20ms loss 1%
sudo tc qdisc add dev enp0s3 parent 1:1 handle 10: tbf rate 20Mbit burst 10mb latency 1ms
