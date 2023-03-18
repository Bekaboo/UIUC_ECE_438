#!/usr/bin/env bash
# vim: ft=sh ts=4 sw=4 sts=4 et :

sudo tc qdisc del dev enp0s3 root 2>/dev/null
