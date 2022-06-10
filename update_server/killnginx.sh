#!/bin/bash
ps -ef | grep nginx | grep -v grep | awk '{print $2}' | xargs kill -9
ps -ef | grep openresty | grep -v grep | awk '{print $2}' | xargs kill -9