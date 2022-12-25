#!/usr/bin/env python

import string
import sys
sys.path.append(r'/usr/local/install/oset/lib')
from esl import *

con = ESLconnection("127.0.0.1","8021","ClueCon")
#are we connected?

if con.connected:
  con.sendRecv("api osetname\n\n")
  con.sendRecv("api hostname\n\n")
  con.sendRecv("log 7\n\n")

  while 1:
  #my $e = $con->recvEventTimed(100);
    e = con.recvEventTimed(100)
    if(e):
        print (e.getBody())