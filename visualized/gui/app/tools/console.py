#!/usr/bin/env python

import string
import os
import sys
sys.path.append(r'/usr/local/install/oset/lib')
from esl import *

import ctypes
import time
import socket
import configparser
import operator
import subprocess
import fcntl
import threading
import queue
import json
import signal
import webbrowser
import io
from contextlib import redirect_stdout
from datetime import datetime

import logging
from logging.config import fileConfig
fileConfig('logging.ini')
logger = logging.getLogger(__name__)

class Esl_Console():
    def __init__(self, ini_file):
        self.path = os.getcwd()
        self.config = configparser.ConfigParser()
        self.config.read(ini_file)

        self.pt_tool_path  = os.path.join(self.path, "tools")        
        self.PT_TARGET_HOST_ADDR   = self.config.get('NETWORK','PT_HOST_ADDR')
        self.PT_TARGET_LISTEN_PORT = self.config.getint('NETWORK','PT_LISTEN_PORT')
        self.PT_PASSWORD           = self.config.get('NETWORK','PT_PASSWORD')
        self.con = ESLconnection(self.PT_TARGET_HOST_ADDR, self.PT_TARGET_LISTEN_PORT ,self.PT_PASSWORD)

    def execute(self):
        if self.con:
            if self.con.connected:
                self.con.sendRecv("api osetname\n\n")
                self.con.sendRecv("api hostname\n\n")
                self.con.sendRecv("log 7\n\n")

                while self.con.connected:
                    if self.con.connected:
                        e = self.con.recvEventTimed(100)
                        if(e):
                            print (e.getBody())
                    else:
                         break
        return
        
if __name__ == '__main__':
    console = Esl_Console('oset.ini')
    console.execute()
    logger.info("Exiting Esl_Console")