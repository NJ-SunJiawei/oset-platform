#!/usr/bin/env python

import json
import os
import re
import sys
import time

from distutils.version import StrictVersion
from functools import partial
from optparse import OptionParser
from subprocess import call
from sys import exit  # pylint: disable=redefined-builtin
import PySimpleGUI as sg

# pylint: disable=import-error
if sys.version_info[0] == 2:
    from Tkinter import ( Frame, Label, LabelFrame, Entry, OptionMenu,
                          Checkbutton, Menu, Toplevel, Button, BitmapImage,
                          PhotoImage, Canvas, Scrollbar, Wm, TclError,
                          StringVar, IntVar, E, W, EW, NW, Y, VERTICAL, SOLID,
                          CENTER, RIGHT, LEFT, BOTH, TRUE, FALSE )
    from ttk import Notebook
    from tkMessageBox import showerror
    import tkFont
    import tkFileDialog
    import tkSimpleDialog
else:
    from tkinter import ( Frame, Label, LabelFrame, Entry, OptionMenu,
                          Checkbutton, Menu, Toplevel, Button, BitmapImage,
                          PhotoImage, Canvas, Scrollbar, Wm, TclError,
                          StringVar, IntVar, E, W, EW, NW, Y, VERTICAL, SOLID,
                          CENTER, RIGHT, LEFT, BOTH, TRUE, FALSE )
    from tkinter.ttk import Notebook
    from tkinter.messagebox import showerror
    from tkinter import font as tkFont
    from tkinter import simpledialog as tkSimpleDialog
    from tkinter import filedialog as tkFileDialog


OPENSET_BASE_PATH = '/usr/local/install/sset'
gl_app_name = None

def graphEditImages():
    return {
        'Select': BitmapImage(
            file='/usr/include/X11/bitmaps/left_ptr' ),

        'OpenSET': PhotoImage( data=r"""
            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
            4BE2eIRYeHAEIBwBP0Y4Qn41YWRSCQgAOw==
            """),

        'OpenCLI': PhotoImage( data=r"""
            R0lGODlhMAAwAPcAAAEBAWfNAYWFhcfHx+3t6/f390lJUaWlpfPz8/Hx72lpaZGRke/v77m5uc0B
            AeHh4e/v7WNjY3t7e5eXlyMjI4mJidPT0+3t7f///09PT7Ozs/X19fHx8ZWTk8HBwX9/fwAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACH5BAEAAAAALAAAAAAwADAA
            Bwj/AAEIHEiwoMGDCBMqXMiwocOHECNKnEixosWLGAEIeMCxo8ePHwVkBGABg8mTKFOmtDByAIYN
            MGPCRCCzQIENNzEMGOkBAwIKQIMKpYCgKAIHCDB4GNkAA4OnUJ9++CDhQ1QGFzA0GKkBA4GvYMOK
            BYtBA1cNaNOqXcuWq8q3b81m7Cqzbk2bMMu6/Tl0qFEEAZLKxdj1KlSqVA3rnet1rOOwiwmznUzZ
            LdzLJgdfpIv3pmebN2Pm1GyRbocNp1PLNMDaAM3Im1/alQk4gO28pCt2RdCBt+/eRg8IP1AUdmmf
            f5MrL56bYlcOvaP7Xo6Ag3HdGDho3869u/YE1507t+3AgLz58ujPMwg/sTBUCAzgy49PH0LW5u0x
            XFiwvz////5dcJ9bjxVIAHsSdUXAAgs2yOCDDn6FYEQaFGDgYxNCpEFfHHKIX4IDhCjiiCSS+CGF
            FlCmogYpcnVABTDGKGOMAlRQYwUHnKjhAjX2aOOPN8LImgAL6PiQBhLMqCSNAThQgQRGOqRBBD1W
            aaOVAggnQARRNqRBBxmEKeaYZIrZQZcMKbDiigqM5OabcMYp55x01ilnQAA7
            """),

        'App': PhotoImage( data=r"""
            R0lGODlhIAAYAPcAMf//////zP//mf//Zv//M///AP/M///MzP/M
            mf/MZv/MM//MAP+Z//+ZzP+Zmf+ZZv+ZM/+ZAP9m//9mzP9mmf9m
            Zv9mM/9mAP8z//8zzP8zmf8zZv8zM/8zAP8A//8AzP8Amf8AZv8A
            M/8AAMz//8z/zMz/mcz/Zsz/M8z/AMzM/8zMzMzMmczMZszMM8zM
            AMyZ/8yZzMyZmcyZZsyZM8yZAMxm/8xmzMxmmcxmZsxmM8xmAMwz
            /8wzzMwzmcwzZswzM8wzAMwA/8wAzMwAmcwAZswAM8wAAJn//5n/
            zJn/mZn/Zpn/M5n/AJnM/5nMzJnMmZnMZpnMM5nMAJmZ/5mZzJmZ
            mZmZZpmZM5mZAJlm/5lmzJlmmZlmZplmM5lmAJkz/5kzzJkzmZkz
            ZpkzM5kzAJkA/5kAzJkAmZkAZpkAM5kAAGb//2b/zGb/mWb/Zmb/
            M2b/AGbM/2bMzGbMmWbMZmbMM2bMAGaZ/2aZzGaZmWaZZmaZM2aZ
            AGZm/2ZmzGZmmWZmZmZmM2ZmAGYz/2YzzGYzmWYzZmYzM2YzAGYA
            /2YAzGYAmWYAZmYAM2YAADP//zP/zDP/mTP/ZjP/MzP/ADPM/zPM
            zDPMmTPMZjPMMzPMADOZ/zOZzDOZmTOZZjOZMzOZADNm/zNmzDNm
            mTNmZjNmMzNmADMz/zMzzDMzmTMzZjMzMzMzADMA/zMAzDMAmTMA
            ZjMAMzMAAAD//wD/zAD/mQD/ZgD/MwD/AADM/wDMzADMmQDMZgDM
            MwDMAACZ/wCZzACZmQCZZgCZMwCZAABm/wBmzABmmQBmZgBmMwBm
            AAAz/wAzzAAzmQAzZgAzMwAzAAAA/wAAzAAAmQAAZgAAM+4AAN0A
            ALsAAKoAAIgAAHcAAFUAAEQAACIAABEAAADuAADdAAC7AACqAACI
            AAB3AABVAABEAAAiAAARAAAA7gAA3QAAuwAAqgAAiAAAdwAAVQAA
            RAAAIgAAEe7u7t3d3bu7u6qqqoiIiHd3d1VVVURERCIiIhEREQAA
            ACH5BAEAAAAALAAAAAAgABgAAAiNAAH8G0iwoMGDCAcKTMiw4UBw
            BPXVm0ixosWLFvVBHFjPoUeC9Tb+6/jRY0iQ/8iVbHiS40CVKxG2
            HEkQZsyCM0mmvGkw50uePUV2tEnOZkyfQA8iTYpTKNOgKJ+C3AhO
            p9SWVaVOfWj1KdauTL9q5UgVbFKsEjGqXVtP40NwcBnCjXtw7tx/
            C8cSBBAQADs=
        """ ),

        'NetLink': PhotoImage( data=r"""
            R0lGODlhFgAWAPcAMf//////zP//mf//Zv//M///AP/M///MzP/M
            mf/MZv/MM//MAP+Z//+ZzP+Zmf+ZZv+ZM/+ZAP9m//9mzP9mmf9m
            Zv9mM/9mAP8z//8zzP8zmf8zZv8zM/8zAP8A//8AzP8Amf8AZv8A
            M/8AAMz//8z/zMz/mcz/Zsz/M8z/AMzM/8zMzMzMmczMZszMM8zM
            AMyZ/8yZzMyZmcyZZsyZM8yZAMxm/8xmzMxmmcxmZsxmM8xmAMwz
            /8wzzMwzmcwzZswzM8wzAMwA/8wAzMwAmcwAZswAM8wAAJn//5n/
            zJn/mZn/Zpn/M5n/AJnM/5nMzJnMmZnMZpnMM5nMAJmZ/5mZzJmZ
            mZmZZpmZM5mZAJlm/5lmzJlmmZlmZplmM5lmAJkz/5kzzJkzmZkz
            ZpkzM5kzAJkA/5kAzJkAmZkAZpkAM5kAAGb//2b/zGb/mWb/Zmb/
            M2b/AGbM/2bMzGbMmWbMZmbMM2bMAGaZ/2aZzGaZmWaZZmaZM2aZ
            AGZm/2ZmzGZmmWZmZmZmM2ZmAGYz/2YzzGYzmWYzZmYzM2YzAGYA
            /2YAzGYAmWYAZmYAM2YAADP//zP/zDP/mTP/ZjP/MzP/ADPM/zPM
            zDPMmTPMZjPMMzPMADOZ/zOZzDOZmTOZZjOZMzOZADNm/zNmzDNm
            mTNmZjNmMzNmADMz/zMzzDMzmTMzZjMzMzMzADMA/zMAzDMAmTMA
            ZjMAMzMAAAD//wD/zAD/mQD/ZgD/MwD/AADM/wDMzADMmQDMZgDM
            MwDMAACZ/wCZzACZmQCZZgCZMwCZAABm/wBmzABmmQBmZgBmMwBm
            AAAz/wAzzAAzmQAzZgAzMwAzAAAA/wAAzAAAmQAAZgAAM+4AAN0A
            ALsAAKoAAIgAAHcAAFUAAEQAACIAABEAAADuAADdAAC7AACqAACI
            AAB3AABVAABEAAAiAAARAAAA7gAA3QAAuwAAqgAAiAAAdwAAVQAA
            RAAAIgAAEe7u7t3d3bu7u6qqqoiIiHd3d1VVVURERCIiIhEREQAA
            ACH5BAEAAAAALAAAAAAWABYAAAhIAAEIHEiwoEGBrhIeXEgwoUKG
            Cx0+hGhQoiuKBy1irChxY0GNHgeCDAlgZEiTHlFuVImRJUWXEGEy
            lBmxI8mSNknm1Dnx5sCAADs=
        """ )
    }

def AppName(): 
    modname=[]
    appname=[] 
    for root, dirs, files in os.walk(OPENSET_BASE_PATH+'/mod'):
      for file in files:
        if os.path.splitext(file)[1] == '.so':
          modname.append(os.path.splitext(file)[0])
    
    for app in modname:
        if None != re.match(r'libapp_',app):
            appname.append(app.split('_')[1])
    return appname  
    
class CustomDialog(object):

    # TODO: Fix button placement and Title and window focus lock
    def __init__(self, master, _title):
        self.top=Toplevel(master)

        self.bodyFrame = Frame(self.top)
        self.bodyFrame.grid(row=0, column=0, sticky='nswe')
        self.body(self.bodyFrame)

        #return self.b # initial focus
        buttonFrame = Frame(self.top, relief='ridge', bd=3, bg='lightgrey')
        buttonFrame.grid(row=1 , column=0, sticky='nswe')

        okButton = Button(buttonFrame, width=8, text='OK', relief='groove',
                   bd=4, command=self.okAction)
        okButton.grid(row=0, column=0, sticky=E)

        canlceButton = Button(buttonFrame, width=8, text='Cancel', relief='groove',
                    bd=4, command=self.cancelAction)
        canlceButton.grid(row=0, column=1, sticky=W)

    def body(self, master):
        self.rootFrame = master

    def apply(self):
        self.top.destroy()

    def cancelAction(self):
        self.top.destroy()

    def okAction(self):
        self.apply()
        self.top.destroy()

class AppDialog(CustomDialog):

    def __init__(self, master, title, prefDefaults):

        self.prefValues = prefDefaults
        self.result = None
        self.appName = AppName()
        CustomDialog.__init__(self, master, title)

    def body(self, master):
        self.rootFrame = master
        #n = Notebook(self.rootFrame)
        #self.propFrame = Frame(n)
        #self.confFrame = Frame(n)
        #n.add(self.propFrame, text='Properties')
        #n.add(self.confFrame, text='Configures')
        #n.pack()

        ### TAB 1
        # Field for Appname
        Label(self.rootFrame, text="App Name:").grid(row=0, column=0, sticky=E)
        self.appTypeVar = StringVar(self.rootFrame)
        self.appTypeMenu = OptionMenu(self.rootFrame, self.appTypeVar, *self.appName)
        self.appTypeMenu.grid(row=0, column=1, sticky=W)
        if 'appname' in self.prefValues:
            self.appTypeVar.set(self.prefValues['appname'])

        Label(self.rootFrame, text="Configure:").grid(row=1, column=0, sticky=E)
        self.b = Button( self.rootFrame, text='set', command=self.cnfInterface)
        self.b.grid(row=1, column=1)

    def cnfInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/%s.conf.xml' % self.appTypeVar.get()
        call("xterm -T '%s Configures' -fn 9*15 -fg black -bg white  -sb -sl 2000 -e 'vim %s' &" % (self.appTypeVar.get() ,xml_files), shell=True)


    def apply(self):
        self.result = {
                   'appname':self.appTypeVar.get(),
                   }


class AppRunDialog(CustomDialog):

    def __init__(self, master, title, prefDefaults):

        self.prefValues = prefDefaults
        self.result = None
        CustomDialog.__init__(self, master, title)

    def body(self, master):
        self.rootFrame = master
        Label(self.rootFrame, text="%s Configure:" % self.prefValues['appname']).grid(row=0, column=0, sticky=E)
        self.b1 = Button( self.rootFrame, text='set', command=self.cnfInterface)
        self.b1.grid(row=0, column=1)

    def cnfInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/%s.conf.xml' % self.prefValues['appname']
        call("xterm -T '%s Configures' -fn 9*15 -fg black -bg white  -sb -sl 2000 -e 'vim %s' &" % (self.prefValues['appname'] ,xml_files), shell=True)


    def apply(self):
        self.result = {}

class OpenSETDialog(CustomDialog):

    def __init__(self, master, title, prefDefaults):
        self.prefValues = prefDefaults
        self.result = None
        CustomDialog.__init__(self, master, title)

    def body(self, master):
        self.rootFrame = master

        rowCount=0
        # Field for Appname
        #Label(self.rootFrame, text="Name:").grid(row=rowCount, column=0, sticky=E)
        #self.appnameEntry = Entry(self.rootFrame)
        #self.appnameEntry.grid(row=rowCount, column=1)
        #self.appnameEntry.insert(0, self.prefValues['appname'])
        #rowCount+=1

        Label(self.rootFrame, text="Core Configure:").grid(row=rowCount, column=0, sticky=E)
        self.b1 = Button( self.rootFrame, text='set', command=self.coreInterface)
        self.b1.grid(row=rowCount, column=1)
        rowCount+=1

        Label(self.rootFrame, text="Module Configure:").grid(row=rowCount, column=0, sticky=E)
        self.b2 = Button( self.rootFrame, text='set', command=self.modulesInterface)
        self.b2.grid(row=rowCount, column=1)
        rowCount+=1

        Label(self.rootFrame, text="Esl Configure:").grid(row=rowCount, column=0, sticky=E)
        self.b3 = Button( self.rootFrame, text='set', command=self.eslInterface)
        self.b3.grid(row=rowCount, column=1)
        rowCount+=1
        
        Label(self.rootFrame, text="Fn Configure:").grid(row=rowCount, column=0, sticky=E)
        self.b4 = Button( self.rootFrame, text='set', command=self.fnInterface)
        self.b4.grid(row=rowCount, column=1)
        rowCount+=1

        Label(self.rootFrame, text="Console Configure:").grid(row=rowCount, column=0, sticky=E)
        self.b5 = Button( self.rootFrame, text='set', command=self.consoleInterface)
        self.b5.grid(row=rowCount, column=1)
        rowCount+=1
        
        Label(self.rootFrame, text="Logfile Configure:").grid(row=rowCount, column=0, sticky=E)
        self.b6 = Button( self.rootFrame, text='set', command=self.logfileInterface)
        self.b6.grid(row=rowCount, column=1)
        rowCount+=1
        
        Label(self.rootFrame, text="Syslog Configure:").grid(row=rowCount, column=0, sticky=E)
        self.b7 = Button( self.rootFrame, text='set', command=self.syslogInterface)
        self.b7.grid(row=rowCount, column=1)
        rowCount+=1
        
        Label(self.rootFrame, text="Credis Configure:").grid(row=rowCount, column=0, sticky=E)
        self.b8 = Button( self.rootFrame, text='set', command=self.credisInterface)
        self.b8.grid(row=rowCount, column=1)
        rowCount+=1
        
        Label(self.rootFrame, text="Hiredis Configure:").grid(row=rowCount, column=0, sticky=E)
        self.b9 = Button( self.rootFrame, text='set', command=self.hiredisInterface)
        self.b9.grid(row=rowCount, column=1)
        rowCount+=1
        
        Label(self.rootFrame, text="Mongo Configure:").grid(row=rowCount, column=0, sticky=E)
        self.b10 = Button( self.rootFrame, text='set', command=self.mongoInterface)
        self.b10.grid(row=rowCount, column=1)
        rowCount+=1

    def coreInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/core.conf.xml'
        call("xterm -T 'core Configures' -fn 9*15 -fg black -bg white -sb -sl 2000 -e 'vim %s' &" % (xml_files), shell=True)

    def modulesInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/modules.conf.xml'
        call("xterm -T 'modules Configures' -fn 9*15 -fg black -bg white  -sb -sl 2000 -e 'vim %s' &" % (xml_files), shell=True)

    def eslInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/event_socket.conf.xml'
        call("xterm -T 'esl Configures' -fn 9*15 -fg black -bg white  -sb -sl 2000 -e 'vim %s' &" % (xml_files), shell=True)

    def fnInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/Fn.conf.xml'
        call("xterm -T 'Fn Configures' -fn 9*15 -fg black -bg white  -sb -sl 2000 -e 'vim %s' &" % (xml_files), shell=True)

    def consoleInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/console.conf.xml'
        call("xterm -T 'console Configures' -fn 9*15 -fg black -bg white  -sb -sl 2000 -e 'vim %s' &" % (xml_files), shell=True)

    def logfileInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/logfile.conf.xml'
        call("xterm -T 'logfile Configures' -fn 9*15 -fg black -bg white  -sb -sl 2000 -e 'vim %s' &" % (xml_files), shell=True)

    def syslogInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/syslog.conf.xml'
        call("xterm -T 'syslog Configures' -fn 9*15 -fg black -bg white  -sb -sl 2000 -e 'vim %s' &" % (xml_files), shell=True)

    def credisInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/credis.conf.xml'
        call("xterm -T 'credis Configures' -fn 9*15 -fg black -bg white  -sb -sl 2000 -e 'vim %s' &" % (xml_files), shell=True)

    def hiredisInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/hiredis.conf.xml'
        call("xterm -T 'hiredis Configures' -fn 9*15 -fg black -bg white  -sb -sl 2000 -e 'vim %s' &" % (xml_files), shell=True)

    def mongoInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/conf/autoload/mongo.conf.xml'
        call("xterm -T 'mongo Configures' -fn 9*15 -fg black -bg white  -sb -sl 2000 -e 'vim %s' &" % (xml_files), shell=True)

    def apply(self):
        self.result = { 
                         #'appname': self.appnameEntry.get(), 
                      }

class OpenCLIDialog(tkSimpleDialog.Dialog):

    def __init__(self, parent, title, ctrlrDefaults=None):
        self.result = None
        self.ctrlrValues = ctrlrDefaults
        tkSimpleDialog.Dialog.__init__(self, parent, title)

    def body(self, master):
        self.rootFrame = master

        rowCount=0
        # Field for Appname
        #Label(self.rootFrame, text="Name:").grid(row=rowCount, column=0, sticky=E)
        #self.appnameEntry = Entry(self.rootFrame)
        #self.appnameEntry.grid(row=rowCount, column=1)
        #self.appnameEntry.insert(0, self.ctrlrValues['appname'])
        #rowCount+=1

        Label(self.rootFrame, text="Cli Configure:").grid(row=rowCount, column=0, sticky=E)
        self.b1 = Button( self.rootFrame, text='set', command=self.cnfInterface)
        self.b1.grid(row=rowCount, column=1)
        rowCount+=1

    def cnfInterface( self ):
        xml_files =  OPENSET_BASE_PATH + '/cli/sset_cli.conf'
        call("xterm -T 'CLI Configures' -fn 9*15 -fg black -bg white -sb -sl 2000 -e 'vim %s' &" % xml_files, shell=True)


    def apply(self):
        self.result = { 
                         #'appname': self.appnameEntry.get(), 
                      }

class LinkDialog(tkSimpleDialog.Dialog):

    def __init__(self, parent, title, linkDefaults):

        self.linkValues = linkDefaults

        tkSimpleDialog.Dialog.__init__(self, parent, title)

    def body(self, master):

        self.var = StringVar(master)
        Label(master, text="Bandwidth:").grid(row=0, sticky=E)
        self.e1 = Entry(master)
        self.e1.grid(row=0, column=1)
        Label(master, text="Mbit").grid(row=0, column=2, sticky=W)
        if 'bw' in self.linkValues:
            self.e1.insert(0,str(self.linkValues['bw']))

        Label(master, text="Delay:").grid(row=1, sticky=E)
        self.e2 = Entry(master)
        self.e2.grid(row=1, column=1)
        if 'delay' in self.linkValues:
            self.e2.insert(0, self.linkValues['delay'])

        Label(master, text="Loss:").grid(row=2, sticky=E)
        self.e3 = Entry(master)
        self.e3.grid(row=2, column=1)
        Label(master, text="%").grid(row=2, column=2, sticky=W)
        if 'loss' in self.linkValues:
            self.e3.insert(0, str(self.linkValues['loss']))

        Label(master, text="Max Queue size:").grid(row=3, sticky=E)
        self.e4 = Entry(master)
        self.e4.grid(row=3, column=1)
        if 'max_queue_size' in self.linkValues:
            self.e4.insert(0, str(self.linkValues['max_queue_size']))

        Label(master, text="Jitter:").grid(row=4, sticky=E)
        self.e5 = Entry(master)
        self.e5.grid(row=4, column=1)
        if 'jitter' in self.linkValues:
            self.e5.insert(0, self.linkValues['jitter'])

        Label(master, text="Speedup:").grid(row=5, sticky=E)
        self.e6 = Entry(master)
        self.e6.grid(row=5, column=1)
        if 'speedup' in self.linkValues:
            self.e6.insert(0, str(self.linkValues['speedup']))

        return self.e1 # initial focus

    def apply(self):
        self.result = {}
        if len(self.e1.get()) > 0:
            self.result['bw'] = int(self.e1.get())
        if len(self.e2.get()) > 0:
            self.result['delay'] = self.e2.get()
        if len(self.e3.get()) > 0:
            self.result['loss'] = int(self.e3.get())
        if len(self.e4.get()) > 0:
            self.result['max_queue_size'] = int(self.e4.get())
        if len(self.e5.get()) > 0:
            self.result['jitter'] = self.e5.get()
        if len(self.e6.get()) > 0:
            self.result['speedup'] = int(self.e6.get())

class ToolTip(object):

    def __init__(self, widget):
        self.widget = widget
        self.tipwindow = None
        self.id = None
        self.x = self.y = 0

    def showtip(self, text):
        "Display text in tooltip window"
        self.text = text
        if self.tipwindow or not self.text:
            return
        x, y, _cx, cy = self.widget.bbox("insert")
        x = x + self.widget.winfo_rootx() + 27
        y = y + cy + self.widget.winfo_rooty() +27
        self.tipwindow = tw = Toplevel(self.widget)
        tw.wm_overrideredirect(1)
        tw.wm_geometry("+%d+%d" % (x, y))
        try:
            # For Mac OS
            # pylint: disable=protected-access
            tw.tk.call("::tk::unsupported::MacWindowStyle",
                       "style", tw._w,
                       "help", "noActivates")
            # pylint: enable=protected-access
        except TclError:
            pass
        label = Label(tw, text=self.text, justify=LEFT,
                      background="#ffffe0", relief=SOLID, borderwidth=1,
                      font=("tahoma", "8", "normal"))
        label.pack(ipadx=1)

    def hidetip(self):
        tw = self.tipwindow
        self.tipwindow = None
        if tw:
            tw.destroy()

class GraphEdit( Frame ):
    def __init__( self, parent=None, cheight=600, cwidth=1000 ):

        Frame.__init__( self, parent )
        self.action = None
        self.appName = 'GraphEdit'
        self.fixedFont = tkFont.Font ( family="DejaVu Sans Mono", size="14" )

        # Style
        self.font = ( 'Geneva', 9 )
        self.smallFont = ( 'Geneva', 7 )
        self.bg = 'white'

        # Title
        self.top = self.winfo_toplevel()
        self.top.title( self.appName )

        # Menu bar
        self.createMenubar()

        # Editing canvas
        self.cheight, self.cwidth = cheight, cwidth
        self.cframe, self.canvas = self.createCanvas()

        # Toolbar
        self.openCLIs = {}

        # Toolbar
        self.images = graphEditImages()
        self.buttons = {}
        self.active = None
        self.tools = ( 'Select', 'App', 'OpenSET', 'OpenCLI', 'NetLink' )
        self.toolbar = self.createToolbar()
        self.nodeBk = {}
        self.nodeBk['OpenSET'] = None
        self.nodeBk['OpenCLI'] = None

        # Layout
        self.toolbar.grid( column=0, row=0, sticky='nsew')
        self.cframe.grid( column=1, row=0 )
        self.columnconfigure( 1, weight=1 )
        self.rowconfigure( 0, weight=1 )
        self.pack( expand=True, fill='both' )

        # About box
        self.aboutBox = None

        # Initialize node data
        self.nodeBindings = self.createNodeBindings()
        self.nodePrefixes = { 'OpenSET': 'Oset PlatForm', 'App': 'App' , 'OpenCLI': 'openCLI'}
        self.widgetToItem = {}
        self.itemToWidget = {}

        # Initialize link tool
        self.link = self.linkWidget = None

        # Selection support
        self.selection = None

        # Keyboard bindings
        self.bind( '<Control-q>', lambda event: self.quit() )
        self.bind( '<KeyPress-Delete>', self.deleteSelection )
        self.bind( '<KeyPress-BackSpace>', self.deleteSelection )
        self.focus()

        self.appPopup = Menu(self.top, tearoff=0)
        self.appPopup.add_command(label='[App Options]', font=self.font)
        self.appPopup.add_separator()
        self.appPopup.add_command(label='Properties', font=self.font, command=self.appDetails )#appDetails

        self.appRunPopup = Menu(self.top, tearoff=0)
        self.appRunPopup.add_command(label='[App Options]', font=self.font)
        self.appRunPopup.add_separator()
        self.appRunPopup.add_command(label='Configures', font=self.font, command=self.appRunDetails )

        self.openSETPopup = Menu(self.top, tearoff=0)
        self.openSETPopup.add_command(label='[PlatForm Options]', font=self.font)
        self.openSETPopup.add_separator()
        self.openSETPopup.add_command(label='Properties', font=self.font, command=self.openSETDetails )#openSETDetails

        self.openSETRunPopup = Menu(self.top, tearoff=0)
        self.openSETRunPopup.add_command(label='[PlatForm Options]', font=self.font)
        self.openSETRunPopup.add_separator()
        self.openSETRunPopup.add_command(label='Module Run Details', font=self.font, command=self.openSETRunDetails )

        self.transLinkPopup = Menu(self.top, tearoff=0)
        self.transLinkPopup.add_command(label='[Link Options]', font=self.font)
        self.transLinkPopup.add_separator()
        self.transLinkPopup.add_command(label='Properties', font=self.font, command=self.transLinkDetails )

        self.appRunLinkPopup = Menu(self.top, tearoff=0)
        self.appRunLinkPopup.add_command(label='[Link Options]', font=self.font)
        self.appRunLinkPopup.add_separator()
        self.appRunLinkPopup.add_command(label='Link Up', font=self.font, command=self.linkUp )
        self.appRunLinkPopup.add_command(label='Link Down', font=self.font, command=self.linkDown )

        self.openCLIPopup = Menu(self.top, tearoff=0)
        self.openCLIPopup.add_command(label='[OpenCLI Options]', font=self.font)
        self.openCLIPopup.add_separator()
        self.openCLIPopup.add_command(label='Properties', font=self.font, command=self.openCLIDetails )

        self.openCLIRunPopup = Menu(self.top, tearoff=0)
        self.openCLIRunPopup.add_command(label='[OpenCLI Options]', font=self.font)
        self.openCLIRunPopup.add_separator()
        self.openCLIRunPopup.add_command(label='CLI Xwin', font=self.font, command=self.openCLIRunDetails )

        # Event handling initalization
        self.linkx = self.linky = self.linkItem = None
        self.lastSelection = None

        # Model initialization
        self.links = {}
        self.appOpts = {}
        self.openSETOpts = {}
        self.appCount = 0
        self.openSETCount = 0
        self.openCLICount = 0
        self.ptRun = False

        # Close window gracefully
        Wm.wm_protocol( self.top, name='WM_DELETE_WINDOW', func=self.quit )

    def quit( self ):
        "Stop our network, if any, then quit."
        self.stop()
        Frame.quit( self )

    def about( self ):
        "Display about box."
        about = self.aboutBox
        if about is None:
            bg = 'white'
            about = Toplevel( bg='white' )
            about.title( 'About' )
            desc = self.appName + ': a simple graph editor for OpenSET'
            version = 'GraphEdit (Beta)1.0'
            author = 'Originally by: Asg Team, September 2022'
            #www = ''
            line1 = Label( about, text=desc, font='Helvetica 10 bold', bg=bg )
            line2 = Label( about, text=version, font='Helvetica 9', bg=bg )
            line3 = Label( about, text=author, font='Helvetica 9', bg=bg )
            #line4 = Entry( about, font='Helvetica 9', bg=bg, width=len(www), justify=CENTER )
            #line4.insert(0, www)
            #line4.configure(state='readonly')
            line1.pack( padx=20, pady=10 )
            line2.pack(pady=10 )
            line3.pack(pady=10 )
            #line4.pack(pady=10 )
            hide = ( lambda about=about: about.withdraw() )
            self.aboutBox = about
            # Hide on close rather than destroying window
            Wm.wm_protocol( about, name='WM_DELETE_WINDOW', func=hide )
        # Show (existing) window
        about.deiconify()

    def createMenubar( self ):
        "Create our menu bar."

        font = self.font

        mbar = Menu( self.top, font=font )
        self.top.configure( menu=mbar )


        fileMenu = Menu( mbar, tearoff=False )
        mbar.add_cascade( label="File", font=font, menu=fileMenu )
        fileMenu.add_command( label="New", font=font, command=self.newTopology )
        #fileMenu.add_separator()
        fileMenu.add_command( label='Quit', command=self.quit, font=font )

        execMenu = Menu( mbar, tearoff=False )
        mbar.add_cascade( label="Exec", font=font, menu=execMenu )
        execMenu.add_command( label="Run", font=font, command=self.doRun )
        execMenu.add_command( label="Stop", font=font, command=self.doStop )
        #fileMenu.add_separator()
        execMenu.add_command( label='Root Terminal', font=font, command=self.rootTerminal )

        # Application menu
        appMenu = Menu( mbar, tearoff=False )
        mbar.add_cascade( label="Help", font=font, menu=appMenu )
        appMenu.add_command( label='About graphEdit', command=self.about, font=font)

    # Canvas
    def createCanvas( self ):
        f = Frame( self )

        canvas = Canvas( f, width=self.cwidth, height=self.cheight,
                         bg=self.bg )

        # Scroll bars
        xbar = Scrollbar( f, orient='horizontal', command=canvas.xview )
        ybar = Scrollbar( f, orient='vertical', command=canvas.yview )
        canvas.configure( xscrollcommand=xbar.set, yscrollcommand=ybar.set )

        # Resize box
        resize = Label( f, bg='white' )

        # Layout
        canvas.grid( row=0, column=1, sticky='nsew')
        ybar.grid( row=0, column=2, sticky='ns')
        xbar.grid( row=1, column=1, sticky='ew' )
        resize.grid( row=1, column=2, sticky='nsew' )

        # Resize behavior
        f.rowconfigure( 0, weight=1 )
        f.columnconfigure( 1, weight=1 )
        f.grid( row=0, column=0, sticky='nsew' )
        f.bind( '<Configure>', lambda event: self.updateScrollRegion() )

        # Mouse bindings
        canvas.bind( '<ButtonPress-1>', self.clickCanvas )
        canvas.bind( '<B1-Motion>', self.dragCanvas )
        canvas.bind( '<ButtonRelease-1>', self.releaseCanvas )

        return f, canvas

    def updateScrollRegion( self ):
        "Update canvas scroll region to hold everything."
        bbox = self.canvas.bbox( 'all' )
        if bbox is not None:
            self.canvas.configure( scrollregion=( 0, 0, bbox[ 2 ],
                                   bbox[ 3 ] ) )

    def canvasx( self, x_root ):
        "Convert root x coordinate to canvas coordinate."
        c = self.canvas
        return c.canvasx( x_root ) - c.winfo_rootx()

    def canvasy( self, y_root ):
        "Convert root y coordinate to canvas coordinate."
        c = self.canvas
        return c.canvasy( y_root ) - c.winfo_rooty()

    # Toolbar

    def activate( self, toolName ):
        "Activate a tool and press its button."
        # Adjust button appearance
        if self.active:
            self.buttons[ self.active ].configure( relief='raised' )
        self.buttons[ toolName ].configure( relief='sunken' )
        # Activate dynamic bindings
        self.active = toolName


    @staticmethod
    def createToolTip(widget, text):
        toolTip = ToolTip(widget)
        def enter(_event):
            toolTip.showtip(text)
        def leave(_event):
            toolTip.hidetip()
        widget.bind('<Enter>', enter)
        widget.bind('<Leave>', leave)

    def createToolbar( self ):
        toolbar = Frame( self )

        # Tools
        for tool in self.tools:
            cmd = ( lambda t=tool: self.activate( t ) )
            b = Button( toolbar, text=tool, font=self.smallFont, command=cmd)
            if tool in self.images:
                b.config( height=35, image=self.images[ tool ] )
                self.createToolTip(b, str(tool))
                # b.config( compound='top' )
            b.pack( fill='x' )
            self.buttons[ tool ] = b
        self.activate( self.tools[ 0 ] )

        # Spacer
        Label( toolbar, text='' ).pack()

        # Commands
        for cmd, color in [ ( 'Stop', 'darkRed' ), ( 'Run', 'darkGreen' ) ]:
            doCmd = getattr( self, 'do' + cmd )
            b = Button( toolbar, text=cmd, font=self.smallFont,
                        fg=color, command=doCmd )
            b.pack( fill='x', side='bottom' )

        return toolbar

    def doRun( self ):
        "Run command."
        self.activate( 'Select' )
        for tool in self.tools:
            self.buttons[ tool ].config( state='disabled' )
        self.start()
        #time.sleep(1)

    def doStop( self ):
        "Stop command."
        self.stop()
        for tool in self.tools:
            self.buttons[ tool ].config( state='normal' )
            if self.openSETCount >= 1:
                self.buttons[ 'OpenSET' ].config( state='disabled' )
            if self.openCLICount >= 1:
                self.buttons[ 'OpenCLI' ].config( state='disabled' )

    def convertJsonUnicode(self, text):
        "Some part don't like Unicode"
        try:
            unicode
        except NameError:
            return text
        if isinstance(text, dict):
            return {self.convertJsonUnicode(key): self.convertJsonUnicode(value) for key, value in text.items()}
        if isinstance(text, list):
            return [self.convertJsonUnicode(element) for element in text]
        if isinstance(text, unicode):  # pylint: disable=undefined-variable
            return text.encode('utf-8')
        return text

    def findWidgetByName( self, name ):
        for widget in self.widgetToItem:
            if name ==  widget[ 'text' ]:
                return widget
        return None

    def newTopology( self ):
        "New command."
        if self.ptRun == True:
            return
        
        for widget in tuple( self.widgetToItem ):
            self.deleteItem( self.widgetToItem[ widget ] )
        self.appCount = 0
        self.openSETCount = 0
        self.openCLICount = 0
        self.links = {}
        self.appOpts = {}
        self.openSETOpts = {}
        self.openCLIs = {}

    # Generic canvas handler
    #
    # We could have used bindtags, as in nodeIcon, but
    # the dynamic approach used here
    # may actually require less code. In any case, it's an
    # interesting introspection-based alternative to bindtags.

    def canvasHandle( self, eventName, event ):
        "Generic canvas event handler"
        if self.active is None:
            return
        toolName = self.active
        handler = getattr( self, eventName + toolName, None )
        if handler is not None:
            handler( event )

    def clickCanvas( self, event ):
        "Canvas click handler."
        self.canvasHandle( 'click', event )

    def dragCanvas( self, event ):
        "Canvas drag handler."
        self.canvasHandle( 'drag', event )

    def releaseCanvas( self, event ):
        "Canvas mouse up handler."
        self.canvasHandle( 'release', event )

    # Currently the only items we can select directly are
    # links. Nodes are handled by bindings in the node icon.

    def findItem( self, x, y ):
        "Find items at a location in our canvas."
        items = self.canvas.find_overlapping( x, y, x, y )
        if len( items ) == 0:
            return None
        else:
            return items[ 0 ]

    # Canvas bindings for Select, App, OpenSET and Link tools

    def clickSelect( self, event ):
        "Select an item."
        self.selectItem( self.findItem( event.x, event.y ) )

    def deleteItem( self, item ):
        "Delete an item."
        # Don't delete while network is running
        if self.buttons[ 'Select' ][ 'state' ] == 'disabled':
            return
        # Delete from model
        if item in self.links:
            self.deleteLink( item )
        if item in self.itemToWidget:
            self.deleteNode( item )
        # Delete from view
        self.canvas.delete( item )

    def deleteSelection( self, _event ):
        "Delete the selected item."
        if self.selection is not None:
            self.deleteItem( self.selection )
        self.selectItem( None )

    def nodeIcon( self, node, name ):
        "Create a new node icon."
        icon = Button( self.canvas, image=self.images[ node ],
                       text=name, compound='top' )
        # Unfortunately bindtags wants a tuple
        bindtags = [ str( self.nodeBindings ) ]
        bindtags += list( icon.bindtags() )
        icon.bindtags( tuple( bindtags ) )
        return icon

    def newNode( self, node, event ):
        "Add a new node to our canvas."
        c = self.canvas
        x, y = c.canvasx( event.x ), c.canvasy( event.y )
        name = self.nodePrefixes[ node ]
        if node == 'OpenSET':
            if self.nodeBk['OpenSET'] == True:
                return
            self.openSETCount += 1
            name = self.nodePrefixes[ node ]
            self.openSETOpts[name] = {}
            self.openSETOpts[name]['nodeNum']=self.openSETCount
            self.openSETOpts[name]['appname']=name
            self.openSETOpts[name]['openCLIs']=[]
            if self.openSETCount >= 1:
                self.buttons[ 'OpenSET' ].config( state='disabled' )
                self.nodeBk['OpenSET'] = True
                
        if node == 'App':
            self.appCount += 1
            name = self.nodePrefixes[ node ] + str( self.appCount )
            self.appOpts[name] = {}
            self.appOpts[name]['nodeNum']=self.appCount
            self.appOpts[name]['appname']=name
        if node == 'OpenCLI':
            if self.nodeBk['OpenCLI'] == True:
                return
            self.openCLICount += 1
            name = self.nodePrefixes[ node ]
            ctrlr = { 'openCLIType': 'ref',
                      'appname': name,}
            self.openCLIs[name] = ctrlr
            # We want to start openCLI count at 0
            if self.openCLICount >= 1:
                self.buttons[ 'OpenCLI' ].config( state='disabled' )
                self.nodeBk['OpenCLI'] = True

        icon = self.nodeIcon( node, name )
        item = self.canvas.create_window( x, y, anchor='c', window=icon,
                                          tags=node )
        self.widgetToItem[ icon ] = item
        self.itemToWidget[ item ] = icon
        self.selectItem( item )
        icon.links = {}
        if node == 'OpenSET':
            icon.bind('<Button-3>', self.do_openSETPopup )
        if node == 'App':
            icon.bind('<Button-3>', self.do_appPopup )
        if node == 'OpenCLI':
            icon.bind('<Button-3>', self.do_openCLIPopup )

    def clickOpenCLI( self, event ):
        "Add a new OpenCLI to our canvas."
        self.newNode( 'OpenCLI', event )

    def clickApp( self, event ):
        "Add a new app to our canvas."
        self.newNode( 'App', event )

    def clickOpenSET( self, event ):
        "Add a new openSET to our canvas."
        self.newNode( 'OpenSET', event )

    def dragNetLink( self, event ):
        "Drag a link's endpoint to another node."
        if self.link is None:
            return
        # Since drag starts in widget, we use root coords
        x = self.canvasx( event.x_root )
        y = self.canvasy( event.y_root )
        c = self.canvas
        c.coords( self.link, self.linkx, self.linky, x, y )

    def releaseNetLink( self, _event ):
        "Give up on the current link."
        if self.link is not None:
            self.canvas.delete( self.link )
        self.linkWidget = self.linkItem = self.link = None

    # Generic node handlers

    def createNodeBindings( self ):
        "Create a set of bindings for nodes."
        bindings = {
            '<ButtonPress-1>': self.clickNode,
            '<B1-Motion>': self.dragNode,
            '<ButtonRelease-1>': self.releaseNode,
            '<Enter>': self.enterNode,
            '<Leave>': self.leaveNode
        }
        l = Label()  # lightweight-ish owner for bindings
        for event, binding in bindings.items():
            l.bind( event, binding )
        return l

    def selectItem( self, item ):
        "Select an item and remember old selection."
        self.lastSelection = self.selection
        self.selection = item

    def enterNode( self, event ):
        "Select node on entry."
        self.selectNode( event )

    def leaveNode( self, _event ):
        "Restore old selection on exit."
        self.selectItem( self.lastSelection )

    def clickNode( self, event ):
        "Node click handler."
        if self.active == 'NetLink':
            self.startLink( event )
        else:
            self.selectNode( event )
        return 'break'

    def dragNode( self, event ):
        "Node drag handler."
        if self.active == 'NetLink':
            self.dragNetLink( event )
        else:
            self.dragNodeAround( event )

    def releaseNode( self, event ):
        "Node release handler."
        if self.active == 'NetLink':
            self.finishLink( event )

    # Specific node handlers

    def selectNode( self, event ):
        "Select the node that was clicked on."
        item = self.widgetToItem.get( event.widget, None )
        self.selectItem( item )

    def dragNodeAround( self, event ):
        "Drag a node around on the canvas."
        c = self.canvas
        # Convert global to local coordinates;
        # Necessary since x, y are widget-relative
        x = self.canvasx( event.x_root )
        y = self.canvasy( event.y_root )
        w = event.widget
        # Adjust node position
        item = self.widgetToItem[ w ]
        c.coords( item, x, y )
        # Adjust link positions
        for dest in w.links:
            link = w.links[ dest ]
            item = self.widgetToItem[ dest ]
            x1, y1 = c.coords( item )
            c.coords( link, x, y, x1, y1 )
        self.updateScrollRegion()

    def createControlLinkBindings( self ):
        "Create a set of bindings for nodes."
        # Link bindings
        # Selection still needs a bit of work overall
        # Callbacks ignore event

        def select( _event, link=self.link ):
            "Select item on mouse entry."
            self.selectItem( link )

        def highlight( _event, link=self.link ):
            "Highlight item on mouse entry."
            self.selectItem( link )
            self.canvas.itemconfig( link, fill='green' )

        def unhighlight( _event, link=self.link ):
            "Unhighlight item on mouse exit."
            self.canvas.itemconfig( link, fill='red' )
            #self.selectItem( None )

        self.canvas.tag_bind( self.link, '<Enter>', highlight )
        self.canvas.tag_bind( self.link, '<Leave>', unhighlight )
        self.canvas.tag_bind( self.link, '<ButtonPress-1>', select )
        
    def createTransferLinkBindings( self ):
        "Create a set of bindings for nodes."
        # Link bindings
        # Selection still needs a bit of work overall
        # Callbacks ignore event

        def select( _event, link=self.link ):
            "Select item on mouse entry."
            self.selectItem( link )

        def highlight( _event, link=self.link ):
            "Highlight item on mouse entry."
            self.selectItem( link )
            self.canvas.itemconfig( link, fill='green' )

        def unhighlight( _event, link=self.link ):
            "Unhighlight item on mouse exit."
            self.canvas.itemconfig( link, fill='magenta' )
            #self.selectItem( None )

        self.canvas.tag_bind( self.link, '<Enter>', highlight )
        self.canvas.tag_bind( self.link, '<Leave>', unhighlight )
        self.canvas.tag_bind( self.link, '<ButtonPress-1>', select )
        self.canvas.tag_bind( self.link, '<Button-3>', self.do_transLinkPopup )

    def createAppLinkBindings( self ):
        "Create a set of bindings for nodes."
        # Link bindings
        # Selection still needs a bit of work overall
        # Callbacks ignore event

        def select( _event, link=self.link ):
            "Select item on mouse entry."
            self.selectItem( link )

        def highlight( _event, link=self.link ):
            "Highlight item on mouse entry."
            self.selectItem( link )
            self.canvas.itemconfig( link, fill='green' )

        def unhighlight( _event, link=self.link ):
            "Unhighlight item on mouse exit."
            self.canvas.itemconfig( link, fill='blue' )
            #self.selectItem( None )

        self.canvas.tag_bind( self.link, '<Enter>', highlight )
        self.canvas.tag_bind( self.link, '<Leave>', unhighlight )
        self.canvas.tag_bind( self.link, '<ButtonPress-1>', select )
        self.canvas.tag_bind( self.link, '<Button-3>', self.do_appLinkPopup )


    def startLink( self, event ):
        "Start a new link."
        if event.widget not in self.widgetToItem:
            # Didn't click on a node
            return

        w = event.widget
        item = self.widgetToItem[ w ]
        x, y = self.canvas.coords( item )
        self.link = self.canvas.create_line( x, y, x, y, width=4,
                                             fill='blue', tag='link' )
        self.linkx, self.linky = x, y
        self.linkWidget = w
        self.linkItem = item


    def finishLink( self, event ):
        "Finish creating a link"
        if self.link is None:
            return
        source = self.linkWidget
        c = self.canvas
        # Since we dragged from the widget, use root coords
        x, y = self.canvasx( event.x_root ), self.canvasy( event.y_root )
        target = self.findItem( x, y )
        dest = self.itemToWidget.get( target, None )
        if ( source is None or dest is None or source == dest
                or dest in source.links or source in dest.links ):
            self.releaseNetLink( event )
            return
        # For now, don't allow apps to be directly linked
        stags = self.canvas.gettags( self.widgetToItem[ source ] )
        dtags = self.canvas.gettags( target )
        # TODO: Make this less confusing
        # pylint: disable=too-many-boolean-expressions
        
        #('App' in stags and 'App' in dtags) or
        if (('OpenCLI' in dtags and 'App' in stags) or
           ('OpenCLI' in stags and 'App' in dtags) or
           ('OpenCLI' in stags and 'OpenCLI' in dtags)):
            self.releaseNetLink( event )
            return

        # Set link type
        linkType='app'
        if 'OpenCLI' in stags or 'OpenCLI' in dtags:
            linkType='control'
            c.itemconfig(self.link, dash=(6, 4, 2, 4), fill='red')
            self.createControlLinkBindings()
        elif 'App' in stags and 'App' in dtags:
            linkType='transfer'
            c.itemconfig(self.link, dash=(6, 4, 2, 4), fill='magenta')
            self.createTransferLinkBindings()            
        else:
            linkType='app'
            c.itemconfig(self.link, dash=(6, 4, 2, 4), fill='blue')
            self.createAppLinkBindings()
        c.itemconfig(self.link, tags=c.gettags(self.link)+(linkType,))

        x, y = c.coords( target )
        c.coords( self.link, self.linkx, self.linky, x, y )
        self.addLink( source, dest, linktype=linkType )
        if linkType == 'control':
            openCLIName = ''
            openSETName = ''
            if 'OpenCLI' in stags:
                openCLIName = source[ 'text' ]
                openSETName = dest[ 'text' ]
            else:
                openCLIName = dest[ 'text' ]
                openSETName = source[ 'text' ]

            self.openSETOpts[openSETName]['openCLIs'].append(openCLIName)

        # We're done
        self.link = self.linkWidget = None

    @staticmethod
    def checkIntf( intf ):
        "Make sure intf exists and is not configured."
        if ( ' %s:' % intf ) not in quietRun( 'ip link show' ):
            showerror(title="Error",
                      message='External interface ' +intf + ' does not exist! Skipping.')
            return False
        ips = re.findall( r'\d+\.\d+\.\d+\.\d+', quietRun( 'ifconfig ' + intf ) )
        if ips:
            showerror(title="Error",
                      message= intf + ' has an IP address and is probably in use! Skipping.' )
            return False
        return True

    def appDetails( self, _ignore=None ):
        if ( self.selection is None or
             self.ptRun == True or
             self.selection not in self.itemToWidget ):
            return
        widget = self.itemToWidget[ self.selection ]
        name = widget[ 'text' ]
        tags = self.canvas.gettags( self.selection )
        if 'App' not in tags:
            return

        prefDefaults = self.appOpts[name]
        appBox = AppDialog(self, title='App Details', prefDefaults=prefDefaults)
        self.master.wait_window(appBox.top)
        if appBox.result:
            newAppOpts = {'nodeNum':self.appOpts[name]['nodeNum']}
            if len(appBox.result['appname']) > 0:
                newAppOpts['appname'] = appBox.result['appname']
                name = appBox.result['appname']
                widget[ 'text' ] = name
            self.appOpts[name] = newAppOpts
            #print( 'New app details for ' + name + ' = ' + str(newAppOpts))

    def openSETDetails( self, _ignore=None ):
        if ( self.selection is None or
             self.ptRun == True or
             self.selection not in self.itemToWidget ):
            return
        widget = self.itemToWidget[ self.selection ]
        name = widget[ 'text' ]
        tags = self.canvas.gettags( self.selection )
        if 'OpenSET' not in tags:
            return

        prefDefaults = self.openSETOpts[name]
        openSETBox = OpenSETDialog(self, title='OpenSET Details', prefDefaults=prefDefaults)
        self.master.wait_window(openSETBox.top)
        if openSETBox.result:
            newOpenSETOpts = {'nodeNum':self.openSETOpts[name]['nodeNum']}
            if len(openSETBox.result['appname']) > 0:
                newOpenSETOpts['appname'] = openSETBox.result['appname']
                name = openSETBox.result['appname']
                widget[ 'text' ] = name
            self.openSETOpts[name] = newOpenSETOpts
            #print( 'New openSET details for ' + name + ' = ' + str(newOpenSETOpts))

    def linkUp( self ):
        if ( self.selection is None or
             self.ptRun == False):
            return
        link = self.selection
        linkDetail =  self.links[link]
        src = linkDetail['src']
        dst = linkDetail['dest']
        srcName, dstName = src[ 'text' ], dst[ 'text' ]
        if re.match(self.nodePrefixes[ 'OpenSET' ], srcName ,re.I) is None:
            cmd = OPENSET_BASE_PATH + '/cli/openCLI -x \'load libapp_%s\'' % srcName
            call(cmd , shell=True)
        elif re.match(self.nodePrefixes[ 'OpenSET' ], dstName ,re.I) is None:
            cmd = OPENSET_BASE_PATH + '/cli/openCLI -x \'load libapp_%s\'' % dstName
            call(cmd , shell=True)
        self.canvas.itemconfig(link, dash=())

    def linkDown( self ):
        if ( self.selection is None or
             self.ptRun == False):
            return
        link = self.selection
        linkDetail =  self.links[link]
        src = linkDetail['src']
        dst = linkDetail['dest']
        srcName, dstName = src[ 'text' ], dst[ 'text' ]
        if re.match(self.nodePrefixes[ 'OpenSET' ], srcName ,re.I) is None:
            cmd = OPENSET_BASE_PATH + '/cli/openCLI -x \'unload libapp_%s\'' % srcName
            call(cmd , shell=True)
        elif re.match(self.nodePrefixes[ 'OpenSET' ], dstName ,re.I) is None:
            cmd = OPENSET_BASE_PATH + '/cli/openCLI -x \'unload libapp_%s\'' % dstName
            call(cmd , shell=True)
        self.canvas.itemconfig(link, dash=(4, 4))

    def transLinkDetails( self, _ignore=None ):
        if ( self.selection is None or
             self.ptRun == True):
            return
        link = self.selection

        linkDetail =  self.links[link]
        # src = linkDetail['src']
        # dest = linkDetail['dest']
        linkopts = linkDetail['linkOpts']
        linkBox = LinkDialog(self, title='Link Details', linkDefaults=linkopts)
        if linkBox.result is not None:
            linkDetail['linkOpts'] = linkBox.result
            #print( 'New link details = ' + str(linkBox.result))

    def openCLIDetails( self ):
        if ( self.selection is None or
             self.ptRun == True or
             self.selection not in self.itemToWidget ):
            return
        widget = self.itemToWidget[ self.selection ]
        name = widget[ 'text' ]
        tags = self.canvas.gettags( self.selection )
        oldName = name
        if 'OpenCLI' not in tags:
            return

        ctrlrBox = OpenCLIDialog(self, title='OpenCLI Details', ctrlrDefaults=self.openCLIs[name])
        if ctrlrBox.result:
            # print( 'OpenCLI is ' + ctrlrBox.result[0])
            if len(ctrlrBox.result['appname']) > 0:
                name = ctrlrBox.result['appname']
                widget[ 'text' ] = name
            else:
                ctrlrBox.result['appname'] = name
            self.openCLIs[name] = ctrlrBox.result
            #print( 'New openCLI details for ' + name + ' = ' + str(self.openCLIs[name]))

    def appRunDetails( self, _ignore=None ):
        if ( self.selection is None or
             self.ptRun == False or
             self.selection not in self.itemToWidget ):
            return
        widget = self.itemToWidget[ self.selection ]
        name = widget[ 'text' ]
        tags = self.canvas.gettags( self.selection )
        if 'App' not in tags:
            return

        prefDefaults = self.appOpts[name]
        appBox = AppRunDialog(self, title='App Details', prefDefaults=prefDefaults)
        self.master.wait_window(appBox.top)

    def openSETRunDetails( self, _ignore=None ):
        if ( self.selection is None or
             self.ptRun == False or
             self.selection not in self.itemToWidget ):
            return
        widget = self.itemToWidget[ self.selection ]
        name = widget[ 'text' ]
        tags = self.canvas.gettags( self.selection )
        if 'OpenSET' not in tags:
            return

        cmd =  OPENSET_BASE_PATH + '/cli/openCLI -x \'show modules\' && read -p \'>>>Press Enter to close<<<\''
        #print(cmd)
        call("xterm -T 'Module Run List' -fn 9*15 -sb -sl 2000 -e \"%s\"  &" %cmd , shell=True)

    def openCLIRunDetails( self ):
        if ( self.selection is None or
             self.ptRun == False or
             self.selection not in self.itemToWidget ):
            return
        widget = self.itemToWidget[ self.selection ]
        name = widget[ 'text' ]
        tags = self.canvas.gettags( self.selection )
        oldName = name
        if 'OpenCLI' not in tags:
            return
            
        cmd =  OPENSET_BASE_PATH + '/cli/openCLI'
        call("xterm -T 'CLI Command' -fn 9*15 -sb -sl 2000 -e \'%s\'  &" %cmd , shell=True)


    @staticmethod
    def rootTerminal( _ignore=None ):
        call(["xterm -T 'Root Terminal' -fn 9*15 -sb -sl 2000 &"], shell=True)

    # Model interface
    #
    # Ultimately we will either want to use a topo or
    # mininet object here, probably.

    def addLink( self, source, dest, linktype='data', linkopts=None ):
        "Add link to model."
        if linkopts is None:
            linkopts = {}
        source.links[ dest ] = self.link
        dest.links[ source ] = self.link
        self.links[ self.link ] = {'type':linktype,
                                   'src':source,
                                   'dest':dest,
                                   'linkOpts':linkopts}

    def deleteLink( self, link ):
        "Delete link from model."
        pair = self.links.get( link, None )
        if pair is not None:
            source=pair['src']
            dest=pair['dest']
            del source.links[ dest ]
            del dest.links[ source ]
            stags = self.canvas.gettags( self.widgetToItem[ source ] )
            # dtags = self.canvas.gettags( self.widgetToItem[ dest ] )
            ltags = self.canvas.gettags( link )

            if 'control' in ltags:
                openCLIName = ''
                openSETName = ''
                if 'OpenCLI' in stags:
                    openCLIName = source[ 'text' ]
                    openSETName = dest[ 'text' ]
                else:
                    openCLIName = dest[ 'text' ]
                    openSETName = source[ 'text' ]

                if openCLIName in self.openSETOpts[openSETName]['openCLIs']:
                    self.openSETOpts[openSETName]['openCLIs'].remove(openCLIName)


        if link is not None:
            del self.links[ link ]

    def deleteNode( self, item ):
        "Delete node (and its links) from model."

        widget = self.itemToWidget[ item ]
        tags = self.canvas.gettags(item)
        if 'OpenCLI' in tags:
            # remove from openSET openCLI lists
            self.openCLICount -= 1
            if self.openCLICount == 0:
                self.buttons[ 'OpenCLI' ].config( state='normal' )
                self.nodeBk['OpenCLI'] = False                

        if 'OpenSET' in tags:
            self.openSETCount -= 1
            if self.openSETCount == 0:
                self.buttons[ 'OpenSET' ].config( state='normal' )
                self.nodeBk['OpenSET'] = False

        for link in tuple( widget.links.values() ):
            # Delete from view and model
            self.deleteItem( link )
        del self.itemToWidget[ item ]
        del self.widgetToItem[ widget ]

    @staticmethod
    def pathCheck( *args, **kwargs ):
        "Make sure each program in *args can be found in $PATH."
        moduleName = kwargs.get( 'moduleName', 'it' )
        for arg in args:
            if not quietRun( 'which ' + arg ):
                showerror(title="Error",
                      message= 'Cannot find required executable %s.\n' % arg +
                       'Please make sure that %s is installed ' % moduleName +
                       'and available in your $PATH.' )

    def buildLinks( self, net):
        # Make links
        print( "Getting Links.\n" )
        for key,link in self.links.items():
            tags = self.canvas.gettags(key)
            if 'app' in tags:
                src=link['src']
                dst=link['dest']
                linkopts=link['linkOpts']
                srcName, dstName = src[ 'text' ], dst[ 'text' ]
                srcNode, dstNode = net.nameToNode[ srcName ], net.nameToNode[ dstName ]
                if linkopts:
                    net.addLink(srcNode, dstNode, cls=TCLink, **linkopts)
                else:
                    # print( str(srcNode) )
                    # print( str(dstNode))
                    net.addLink(srcNode, dstNode)
                self.canvas.itemconfig(key, dash=())
        

    def start( self ):
        "Start network."
        if self.ptRun == False:
            self.ptRun = True
            clearCmd = OPENSET_BASE_PATH + '/log'
            call('cd %s && rm -rf sset.log*' % clearCmd, shell=True)

            stopCmd = OPENSET_BASE_PATH + '/run/openSET -stop'
            call(stopCmd, shell=True)
            time.sleep(1)
            runCmd = OPENSET_BASE_PATH + '/run/openSET -nc'
            call(runCmd, shell=True)

            t_end = time.time() + 5
            while time.time() < t_end:
                for item in self.links:
                    if self.canvas.gettags(item)[1] == 'control':
                        self.canvas.itemconfig(item, dash=(), fill='red')

    def stop( self ):
        "Stop network."
        if self.ptRun == True:
            # Stop app details
            stopCmd = OPENSET_BASE_PATH + '/run/openSET -stop'
            call(stopCmd, shell=True)
            for item in self.links:
                #print(self.canvas.gettags(item))
                if self.canvas.gettags(item)[1] == 'app':
                    self.canvas.itemconfig(item, dash=(6, 4, 2, 4), fill='blue')
                elif self.canvas.gettags(item)[1] == 'control':
                    self.canvas.itemconfig(item, dash=(6, 4, 2, 4), fill='red')
        self.ptRun = False

    def do_transLinkPopup(self, event):
        # display the popup menu
        if self.ptRun == False:
            try:
                self.transLinkPopup.tk_popup(event.x_root, event.y_root, 0)
            finally:
                # make sure to release the grab (Tk 8.0a1 only)
                self.transLinkPopup.grab_release()

                
    def do_appLinkPopup(self, event):
        # display the popup menu
        if self.ptRun == True:
            try:
                self.appRunLinkPopup.tk_popup(event.x_root, event.y_root, 0)
            finally:
                # make sure to release the grab (Tk 8.0a1 only)
                self.appRunLinkPopup.grab_release()

    def do_openCLIPopup(self, event):
        # display the popup menu
        if self.ptRun == False:
            try:
                self.openCLIPopup.tk_popup(event.x_root, event.y_root, 0)
            finally:
                # make sure to release the grab (Tk 8.0a1 only)
                self.openCLIPopup.grab_release()
        else:
            try:
                self.openCLIRunPopup.tk_popup(event.x_root, event.y_root, 0)
            finally:
                # make sure to release the grab (Tk 8.0a1 only)
                self.openCLIRunPopup.grab_release()

    def do_appPopup(self, event):
        # display the popup menu
        if self.ptRun == False:
            try:
                self.appPopup.tk_popup(event.x_root, event.y_root, 0)
            finally:
                # make sure to release the grab (Tk 8.0a1 only)
                self.appPopup.grab_release()
        else:
            try:
                self.appRunPopup.tk_popup(event.x_root, event.y_root, 0)
            finally:
                # make sure to release the grab (Tk 8.0a1 only)
                self.appRunPopup.grab_release()

    def do_openSETPopup(self, event):
        # display the popup menu
        if self.ptRun == False:
            try:
                self.openSETPopup.tk_popup(event.x_root, event.y_root, 0)
            finally:
                # make sure to release the grab (Tk 8.0a1 only)
                self.openSETPopup.grab_release()
        else:
            try:
                self.openSETRunPopup.tk_popup(event.x_root, event.y_root, 0)
            finally:
                # make sure to release the grab (Tk 8.0a1 only)
                self.openSETRunPopup.grab_release()

    ### BELOW HERE IS THE TOPOLOGY IMPORT CODE ###

    def parseArgs( self ):
        """Parse command-line args and return options object.
           returns: opts parse options dict"""

        if '--custom' in sys.argv:
            index = sys.argv.index( '--custom' )
            if len( sys.argv ) > index + 1:
                filename = sys.argv[ index + 1 ]
                self.parseCustomFile( filename )
            else:
                raise Exception( 'Custom file name not found' )

        desc = ( "The %prog utility creates Mininet network from the\n"
                 "command line. It can create parametrized topologies,\n"
                 "invoke the Mininet CLI, and run tests." )

        usage = ( '%prog [options]\n'
                  '(type %prog -h for details)' )

        opts = OptionParser( description=desc, usage=usage )

        addDictOption( opts, TOPOS, TOPODEF, 'topo' )
        addDictOption( opts, LINKS, LINKDEF, 'link' )

        opts.add_option( '--custom', type='string', default=None,
                         help='read custom topo and node params from .py' +
                         'file' )

        self.options, self.args = opts.parse_args()
        # We don't accept extra arguments after the options
        if self.args:
            opts.print_help()
            exit()

    def setCustom( self, name, value ):
        "Set custom parameters for MininetRunner."
        if name in ( 'topos', 'openSETes', 'apps', 'openCLIs' ):
            # Update dictionaries
            param = name.upper()
            globals()[ param ].update( value )
        elif name == 'validate':
            # Add custom validate function
            self.validate = value
        else:
            # Add or modify global variable or class
            globals()[ name ] = value

    def parseCustomFile( self, fileName ):
        "Parse custom file and add params before parsing cmd-line options."
        customs = {}
        if os.path.isfile( fileName ):
            with open( fileName, 'r' ) as f:
                exec( f.read() )  # pylint: disable=exec-used
            for name, val in customs.items():
                self.setCustom( name, val )
        else:
            raise Exception( 'could not find custom file: %s' % fileName )


if __name__ == '__main__':
    gl_app_name = AppName()
    app = GraphEdit()
    app.mainloop()    
