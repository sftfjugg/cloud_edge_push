# -*- coding: utf-8 -*-

import time
import json
from shm_buf import SharedMemoryBuffer
import psutil
import datetime

def brief_info():
    cpu_percent = psutil.cpu_percent()
    memory_percent = psutil.virtual_memory()[2]
    disk_info = ""
    for part in psutil.disk_partitions(all=False):
        usage = psutil.disk_usage(part.mountpoint)
        disk_info += str("  %s used %s percent. \n" % (part.device, usage.percent))
    boot_time = datetime.datetime.fromtimestamp(psutil.boot_time())
    running_since = boot_time.strftime("%A %d. %B %Y")
    
    t = {}
    t["cpu_percent"] = cpu_percent
    t["memory_percent"] = memory_percent
    t["disk_partitions"] = psutil.disk_partitions(all=False)
    t["boot_time"] = str(boot_time)
    return t


def timing_send_sysinfo():
    shm_name = "edge_client_cpp_py"
    shm_buf = SharedMemoryBuffer(shm_name = shm_name, create_shm = True)
    
    while True:
        info = brief_info()
        shm_buf.write_shm(json.dumps(info, ensure_ascii=False).encode('utf-8'))
        time.sleep(10)


if __name__ == "__main__":
    timing_send_sysinfo()
