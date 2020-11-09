#!/bin/bash
ps ax -o comm | grep -v grep | grep -v sys-mon.sh | grep -q sys-mon || $(dirname "$0")/sys-mon $1
