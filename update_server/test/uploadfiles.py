# -*- coding: utf-8 -*-

import os
import requests
import time
import json

class APITest:
    def __init__(self):
        self.srv_url = "http://192.21.1.61:9988"
        pass

    def uploadfiles(self):

        url = self.srv_url + '/sys/v1/service_upload'
        files = {'libunqlite.so': open('../lib/libunqlite.so', 'rb'), 'http.log': open('../logs/http.log', 'rb')}
        data =[{"filename":"libunqlite.so","version":"1.0.1","type":"SUZHOU"},
                {"filename":"http.log","version":"1.0.1","type":"BEIJING"}]
        r = requests.post(url, data={"files_info" : json.dumps(data)}, files=files, timeout=(10, 60))
        print(f'url: \n{url}\nrespone:\n{r.text}\n')

if __name__ == "__main__":
    api_test = APITest()
    api_test.uploadfiles()
