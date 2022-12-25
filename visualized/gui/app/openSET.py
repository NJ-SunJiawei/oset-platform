#!/usr/bin/env python

import os
import sys
sys.path.append(r'/usr/local/install/oset/lib')

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

import PySimpleGUI as sg
from plugin import CreateApp, ManageTutorials, crc_32c, datagram_to_str, compress_abs_path, TextEditor
from plugin import display_notification, HashGeneratorGUI, PasswordMatches
from plugin import AppStore, ManageUsrApps, AppSpec
from esl import *


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
#=====================================#

class ManageCfs():
    """
    Manage the display for building and running the cFS.
    app_abs_path - Is the python application, not cFS apps
    """
    def __init__(self, pt_abs_path, usr_app_rel_path, main_window):
        self.pt_abs_path    = pt_abs_path
        #self.cfs_abs_base_path = cfs_abs_base_path
        #self.cfs_abs_defs_path = os.path.join(self.cfs_abs_base_path, "cfsat_defs")     #TODO - Use constants
        self.exit_tools_path  = os.path.join(pt_abs_path, "tools")
        self.usr_app_path      = compress_abs_path(os.path.join(pt_abs_path, usr_app_rel_path))
        self.main_window       = main_window
        self.build_subprocess  = None
        self.install_subprocess  = None
        self.uninstall_subprocess  = None
        self.selected_app      = None
        
        self.b_size  = (2,1)
        self.b_pad   = ((0,2),(2,2))
        self.b_font  = ('Arial bold', 11)
        self.b_color = 'red'
        self.t_font  = ('Arial', 14)

    def select_app_gui(self, app_name_list):
        """
        Select an app to be integrated
        """
        self.selected_app = None
        
        layout = [
                  [sg.Text('Select an app from the dropdown iist and click OK\n', font=self.b_font)],
                  [sg.Combo(app_name_list, pad=self.b_pad, font=self.b_font, enable_events=True, key="-USR_APP-", default_value=app_name_list[0]),
                   sg.Button('Submit', button_color=('white', '#007339'), pad=self.b_pad, key='-SUBMIT-'),
                   sg.Button('Cancel', button_color=('white', 'firebrick4'), pad=self.b_pad, key='-CANCEL-')]
                 ]      

        window = sg.Window('Select App to Integrate', layout, resizable=True, modal=True)
        
        while True:
        
            event, values = window.read()
        
            if event in (sg.WIN_CLOSED, '-CANCEL-') or event is None:
                break
                
            elif event == '-SUBMIT-':
                self.selected_app = values['-USR_APP-']
                break
        
        window.close()       
              
    
    def integrate_app_gui(self, usr_app_spec):
        """
        Provide steps for the user to integrate an app. This is only invoked
        after an app has been selected.
        The steps have some degree of independence in case the user doesn't do
        things in order which means some processing may be repeated. For example
        the table files are recomputed for the edit targets.cmake step and the
        copy files to cfsat_defs steps. 
        """
        layout = [
                  [sg.MLine(default_text='-- No Messages Received --', enable_events=True, size=(100,30),key='-BUILD_CMAKE_TEXT-')],
                  [sg.Button('Edit', button_color=('white', '#007339'), enable_events=True, key='-Edit-', pad=((10,10),(12,12))),
                   #sg.Button('Build', enable_events=True, key='-BUILD-', pad=((10,10),(12,12))),
                   sg.Button('Install', enable_events=True, key='-INSTALL-', pad=((10,10),(12,12))),
                   sg.Button('Uninstall', enable_events=True, key='-UNINSTALL-', pad=((10,10),(12,12))),
                   sg.Button('Remove', button_color=self.b_color, enable_events=True, key='-REMOVE_APP-', pad=((10,10),(12,12))),
                   sg.Button('Exit', button_color=('white', 'firebrick4'), enable_events=True, key='-BUILD_CMAKE_EXIT-', pad=((10,10),(12,12)))]
                 ]
        self.window = sg.Window('Integrate %s with the OpenSET' % usr_app_spec.app_name, layout, 
                                                            auto_size_text=True,
                                                            grab_anywhere=True, 
                                                            margins=(0,0),
                                                            #resizable=True,
                                                            modal=True)
        
        while True:
        
            self.event, self.values = self.window.read()
        
            if self.event in (sg.WIN_CLOSED, 'Exit', '-BUILD_CMAKE_EXIT-') or self.event is None:
                #if self.build_subprocess is not None:
                #    if self.build_stdout is not None:
                #        self.build_stdout.terminate()  # I tried to join() afterwards and it hangs
                if self.install_subprocess is not None:
                    os.killpg(self.install_subprocess.pid, signal.SIGKILL)
                    if self.install_subprocess.poll() is None:
                        os.killpg(self.install_subprocess.pid, signal.SIGKILL)
                    if self.install_stdout is not None:
                        self.install_stdout.terminate()  # I tried to join() afterwards and it hangs
                if self.uninstall_subprocess is not None:
                    os.killpg(self.uninstall_subprocess.pid, signal.SIGKILL)
                    if self.uninstall_subprocess.poll() is None:
                        os.killpg(self.uninstall_subprocess.pid, signal.SIGKILL)
                    if self.uninstall_stdout is not None:
                        self.uninstall_stdout.terminate()  # I tried to join() afterwards and it hangs
                self.install_subprocess = None
                self.uninstall_subprocess = None
                break

            elif self.event == '-Edit-':
                path_filename = os.path.join(self.usr_app_path, usr_app_spec.app_name ,'CMakeLists.txt')
                self.text_editor = sg.execute_py_file("texteditor.py", parms=path_filename, cwd=self.exit_tools_path)

            elif self.event == '-REMOVE_APP-': # Build
                remove_path = os.path.join(self.usr_app_path, usr_app_spec.app_name)
                res = os.system('rm -rf %s' % remove_path)
                if res == 0:
                    self.window['-BUILD_CMAKE_TEXT-'].update(' ')
                    sg.popup('Remove app %s success!' % usr_app_spec.app_name, title='Success', grab_anywhere=True, modal=True)
                else:
                    sg.popup_error('Remove app %s fail!' % usr_app_spec.app_name, title='Error', grab_anywhere=True, modal=True)   

            elif self.event == '-INSTALL-':
                if self.install_subprocess is None:
                    install_path = os.path.join(self.usr_app_path, usr_app_spec.app_name ,'build')
                    if not os.path.exists(install_path):
                        os.makedirs(install_path)
                    self.install_subprocess = subprocess.Popen('cd %s && cmake .. && make install' % install_path,
                                                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, bufsize=1, universal_newlines=True, start_new_session=True)
                    if self.install_subprocess is not None:
                        self.install_stdout = PopenStdout(self.install_subprocess, self.window, '-BUILD_CMAKE_TEXT-', True)#self.main_window
                        self.install_stdout.start()
                else:
                    sg.popup_error("Duplicate Install ", title='Warning', grab_anywhere=True, modal=True)

            elif self.event == '-UNINSTALL-':
                if self.uninstall_subprocess is None:
                    uninstall_path = os.path.join(self.usr_app_path, usr_app_spec.app_name ,'build')
                    self.uninstall_subprocess = subprocess.Popen('cd %s && make uninstall' % uninstall_path,
                                                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, bufsize=1, universal_newlines=True, start_new_session=True)
                    if self.uninstall_subprocess is not None:
                        self.uninstall_stdout = PopenStdout(self.uninstall_subprocess, self.window, '-BUILD_CMAKE_TEXT-', True)#self.main_window
                        self.uninstall_stdout.start()

        self.window.close()       

    def execute(self):
        if not os.path.exists(self.usr_app_path):
            os.makedirs(self.usr_app_path)
        self.manage_usr_apps = ManageUsrApps(self.usr_app_path)
        self.cfs_app_specs = self.manage_usr_apps.get_app_specs()
        if len(self.cfs_app_specs) > 0:
            self.select_app_gui(list(self.cfs_app_specs.keys()))
            if self.selected_app is not None:
                self.integrate_app_gui(self.manage_usr_apps.get_app_spec(self.selected_app))
        else:
            sg.popup('Your usr/apps directory is empty', title='Error', grab_anywhere=True, modal=True)

###############################################################################
class ConsoleStdout(threading.Thread):
    """
    """
    def __init__(self, con, window, key):
        threading.Thread.__init__(self)
        self.con = con
        self.window = window
        self.key = key
        self.pt_console_log = ""
        
    def run(self):
        try:
            logger.info("Starting pt console window stdout display")
            if self.con.connected():
                self.con.sendRecv("api osetname\n\n")
                self.con.sendRecv("api hostname\n\n")
                self.con.sendRecv("log 7\n\n")
                #self.con.events("plain", "all");
            else:
                return

            while self.con.connected():
                e = self.con.recvEventTimed(1)
                if(e):
                    #print(">>Line: " + e.getBody())
                    self.pt_console_log += e.getBody()
                    self.window[self.key].update(self.pt_console_log)
                    self.window[self.key].set_vscroll_position(1.0)  # Scroll to bottom (most recent entry)
            return
        finally:
            logger.info("Stopping pt console window stdout display")
            
    def get_id(self):
 
        # returns id of the respective thread
        if hasattr(self, '_thread_id'):
            return self._thread_id
        for id, thread in threading._active.items():
            if thread is self:
                return id
  
    def terminate(self):
        """
        Terminate the thread by rasing an exception
        """
        logger.info("Raising PopenStdout exception to terminate thread")
        thread_id = self.get_id()
        res = ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id,
              ctypes.py_object(SystemExit))
        if res > 1:
            ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id, 0)
            print('Exception raise failure')


###############################################################################
class PopenStdout(threading.Thread):
    """
    """
    def __init__(self, pt_subprocess, window, key, err_out_flag):
        threading.Thread.__init__(self)
        self.pt_subprocess = pt_subprocess
        self.window = window
        self.pt_subprocess_log = ""
        self.key = key
        self.err_out_flag = err_out_flag
        
    def run(self):
        """
        This function is invoked after a OSET process is started and it's design depends on how Popen is
        configured when the Oset process is started. I've tried lots of different designs to make this 
        non-blocking and easay to terminate. It assumes the the Popen parameters bufsize=1 and
        universal_newlines=True (text output). A binary stdout would need line.decode('utf-8'). Some loop
        design options:
            for line in io.TextIOWrapper(self.pt_subprocess.stdout, encoding="utf-8"):
                self.pt_subprocess_log += line

            while True:
                line = self.pt_subprocess.stdout.readline()
                if not line:
                    break
                self.pt_subprocess_log += line

            for line in iter(self.pt_subprocess.stdout.readline, ''):
                print(">>Line: " + line)
                self.pt_subprocess_log += line

        Reading stdout is a blocking function. The current design does not let the process get killed and I
        think it's because the read function is always active. I put the try block there becuase I'd like to
        add an exception mechanism to allow the thread to be terminated. Subprocess communiate with a timeout
        i not an option becuase the child process is terminated if a tomeout occurs.I tried the psuedo terminal
        module as an intermediator between the OSET process and stdout thinking it may have a non-blocking but
         it still blocked. 
        
        """
        try:
            logger.info("Starting pt cmake window stdout display")
            for line in iter(self.pt_subprocess.stdout.readline, ''):
                #print(">>Line: " + line)
                self.pt_subprocess_log += line
                self.window[self.key].update(self.pt_subprocess_log)
                self.window[self.key].set_vscroll_position(1.0)  # Scroll to bottom (most recent entry)
            if self.err_out_flag:
                for line_err in iter(self.pt_subprocess.stderr.readline, ''):
                    #print(">>Line: " + line_err)
                    self.pt_subprocess_log += line_err
                    self.window[self.key].update(self.pt_subprocess_log)
                    self.window[self.key].set_vscroll_position(1.0)  # Scroll to bottom (most recent entry)
        finally:
            logger.info("Stopping pt cmake window stdout display")
            
    def get_id(self):
 
        # returns id of the respective thread
        if hasattr(self, '_thread_id'):
            return self._thread_id
        for id, thread in threading._active.items():
            if thread is self:
                return id
  
    def terminate(self):
        """
        Terminate the thread by rasing an exception
        """
        logger.info("Raising PopenStdout exception to terminate thread")
        thread_id = self.get_id()
        res = ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id,
              ctypes.py_object(SystemExit))
        if res > 1:
            ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id, 0)
            print('Exception raise failure')

###############################################################################
class App():
    GUI_NULL_TXT     = 'Null'
    def __init__(self, ini_file):

        self.path = os.getcwd()
        self.config = configparser.ConfigParser()
        self.config.read(ini_file)

        sg.theme('SystemDefault') #SystemDefaultForReal

        self.pt_banner_log =  ".=============================================================.\n|  _________       _________      _________      ___________  |\n| |  _____  |     |  _____  |    |  _______|    |___     ___| |\n| | |     | |     | |     |_|    | |                 | |      |\n| | |     | |     | |_______     | |_______          | |      |\n| | |     | |     |_______  |    |  _______|         | |      |\n| | |     | |      _      | |    | |                 | |      |\n| | |_____| |     | |_____| |    | |_______          | |      |\n| |_________|     |_________|    |_________|         |_|      |\n|                                                             |\n.=============================================================.\n| SSET Platform create by SSET protocol team                  |\n.=============================================================.\n\n"
        self.pt_initial_log = '-- No Messages Received --'

        self.pt_tool_path  = os.path.join(self.path, "tools")
        self.pt_abs_base_path  = self.config.get('SSET_TARGET','BASE_PATH')
        self.pt_abs_log_path  = self.config.get('SSET_TARGET','LOG_PATH')
        self.PT_HASH_KEY     = self.config.get('SSET_TARGET','HASH_KEY')
        self.pt_subprocess     = None
        self.pt_window_stdout = None
        self.pt_start_subprocess  = None
        self.pt_time_event_filter = False
        self.esl_con  = None
        self.esl_con_success_flag  = False 
        
        self.history_log_files = []

        self.APP_VERSION = self.config.get('APP','VERSION')

        self.PT_TARGET_HOST_ADDR   = self.config.get('NETWORK','PT_HOST_ADDR')
        self.PT_TARGET_LISTEN_PORT = self.config.getint('NETWORK','PT_LISTEN_PORT')
        self.PT_PASSWORD           = self.config.get('NETWORK','PT_PASSWORD')

        self.event_log   = ""        
        self.event_queue = queue.Queue()
        self.window = None
                
        self.manage_tutorials = ManageTutorials(self.config.get('PATHS', 'TUTORIALS_PATH'))
        self.create_app       = CreateApp(self.config.get('PATHS', 'APP_TEMPLATES_PATH'), self.config.get('PATHS', 'USR_APP_PATH'))

        self.tutorial      = None
        
        self.command_history = []
        self.history_offset = 0

        self.gnb_slice_sim_subprocess = None
        self.gnb_slice_sim_stdout = None

    def update_event_history_str(self, new_event_text):
        time = datetime.now().strftime("%H:%M:%S")
        event_str = time + " - " + new_event_text + "\n"        
        self.event_log += event_str
   
    def display_event(self, new_event_text):
        self.update_event_history_str(new_event_text)
        self.window['-EVENT_TEXT-'].update(self.event_log)
        self.window['-EVENT_TEXT-'].set_vscroll_position(1.0)

    def ComingSoonPopup(self, feature_str):
        sg.popup(feature_str, title='Coming soon...', grab_anywhere=True, modal=False)

    def update_history_log(self):
        history_log_file = 'sset_'+datetime.now().strftime("%H:%M:%S")+'.log'
        subprocess.call('cd %s && cp sset.log %s' % (self.pt_abs_log_path, history_log_file), shell=True)
        self.history_log_files.append(history_log_file)
        subprocess.call('cd %s && rm -rf sset.log' % self.pt_abs_log_path, shell=True)
        if(len(self.history_log_files) == (10 + 1)):
            subprocess.call('cd %s && rm -rf %s' % (self.pt_abs_log_path, self.history_log_files.pop(0)), shell=True)

    def shutdown(self):
        logger.info("Starting app shutdown sequence")
        self.window.close()
        logger.info("Completed app shutdown sequence")    
    def Collapsible(self, layout, key, title='', arrows=(sg.SYMBOL_DOWN, sg.SYMBOL_UP), collapsed=False):
        """
        User Defined Element
        A "collapsable section" element. Like a container element that can be collapsed and brought back
        :param layout:Tuple[List[sg.Element]]: The layout for the section
        :param key:Any: Key used to make this section visible / invisible
        :param title:str: Title to show next to arrow
        :param arrows:Tuple[str, str]: The strings to use to show the section is (Open, Closed).
        :param collapsed:bool: If True, then the section begins in a collapsed state
        :return:sg.Column: Column including the arrows, title and the layout that is pinned
        """
        return sg.Column([[sg.T((arrows[1] if collapsed else arrows[0]), enable_events=True, k=key+'-BUTTON-'),
                           sg.T(title, enable_events=True, key=key+'-TITLE-')],
                          [sg.pin(sg.Column(layout, key=key, visible=not collapsed, metadata=arrows))]], pad=(0,0))
                        
    def create_window(self, sys_target_str, sys_comm_str):
        """
        Create the main window. Non-class variables are used so it can be refreshed, PySimpleGui
        layouts can't be shared.
        """
        sg.set_options(element_padding=(0, 0))
    
        menu_def = [
                       ['System', ['Options', ['OSET setting', 'Logging setting'], 'About', '&Exit']],
                       ['Developer', ['&Create App', '!&Download App', '&Install/Uninstall', '---', '!Run Perf Monitor']],
                       ['Operator', ['&Graph Edit','&Files Edit', '!Manage Tables']],
                       ['Documents', ['OSET Overview']], #todo
                       ['Tutorials', self.manage_tutorials.tutorial_titles]
                   ]
        right_click_menu_def = [[],['&Graph Edit', '&Install/Uninstall','---', '&Exit']]

        self.esl_config_cmds = ['-- ESL Configuration--', 'Connect OpenSET', 'Disconnect OpenSET']


        # Events can't be posted until after first window.read() so initialization string is format here and used as the default string     
        self.update_event_history_str(sys_target_str)
        self.update_event_history_str(sys_comm_str)
        
        pri_hdr_font = ('Arial bold',14)
        sec_hdr_font = ('Arial',12)
        log_font = ('Courier',12)

        toolbar_buttons = [
           [sg.Button('', image_data=close64[22:],button_color=('white', sg.COLOR_SYSTEM_DEFAULT), pad=(0,0), key='-close-'),
            sg.Button('', image_data=checkmark64[22:], button_color=('white', sg.COLOR_SYSTEM_DEFAULT), pad=(0, 0), key='-checkmark-'),
            sg.Button('', image_data=cookbook64[22:], button_color=('white', sg.COLOR_SYSTEM_DEFAULT), pad=(0, 0), key='-cookbook-'),
            sg.Button('', image_data=download64[22:], button_color=('white', sg.COLOR_SYSTEM_DEFAULT), pad=(0, 0), key='-download-'),
            sg.Button('', image_data=github64[22:], button_color=('white', sg.COLOR_SYSTEM_DEFAULT), pad=(0, 0), key='-github-'),
            sg.Button('', image_data=run64[22:], button_color=('white', sg.COLOR_SYSTEM_DEFAULT), pad=(0, 0), key='-run-'),
            sg.Button('', image_data=storage64[22:], button_color=('white', sg.COLOR_SYSTEM_DEFAULT), pad=(0, 0), key='-storage-'),
        ]]

        section_console = [
                     #[sg.Frame('', toolbar_buttons, title_color='white', title_location=sg.TITLE_LOCATION_TOP_LEFT ,background_color=sg.COLOR_SYSTEM_DEFAULT, pad=(0, 0))],
                     [sg.Button('Start PT', enable_events=True, key='-START_OSET_PT-',  image_data=green, image_subsample=2, button_color=('black', sg.theme_background_color()), border_width=0),
                      sg.Button('Stop PT', enable_events=True, key='-STOP_OSET_PT-', image_data=red, image_subsample=2, button_color=('black', sg.theme_background_color()), border_width=0),
                      sg.Text('ESL Config:', font=sec_hdr_font, pad=((0,0),(12,12))),
                      sg.Combo(self.esl_config_cmds, enable_events=True, key="-ESL_CONFIG_CMD-", default_value=self.esl_config_cmds[0], pad=((0,5),(12,12))),
                      sg.Text('CLI:', font=sec_hdr_font, pad=((5,0),(12,12))),
                      #sg.Input('show help' ,enable_events=True, key='-CLI_TEXT-', pad=((5,0),(12,12)), do_not_clear=False,),
                      sg.MLine('show help',enter_submits=True, key='-CLI_TEXT-', do_not_clear=False,  pad=((5,0),(12,12))),
                      sg.Button('Send', enable_events=True, key='-SEND_CLI_TEXT-', pad=((5,5),(12,12))),
                      sg.Button('History', enable_events=True, key='-CLI_HISTROY_TEXT-', pad=((5,5),(12,12)))],
                     #[sg.Image(data=sg.DEFAULT_BASE64_LOADING_GIF, enable_events=True, key='-GIF_LOAD_IMAGE-',pad=((10,5),(12,12))),],
                     [sg.Text('Platform Window', font=pri_hdr_font),sg.Button('Clear', enable_events=True, key='-CLEAR_PT_PROCESS_TEXT-', pad=(5,1))],
                     #[sg.Output(font=log_font, size=(115, 10), key='-PT_PROCESS_TEXT-')],
                     [sg.MLine(default_text=self.pt_initial_log, font=log_font, enable_events=True, size=(120, 20), autoscroll=True, key='-PT_PROCESS_TEXT-', expand_x=True, expand_y=True)],
                     [sg.Text('Events Window', font=pri_hdr_font), sg.Button('Clear', enable_events=True, key='-CLEAR_EVENTS-', pad=(5,1))],
                     [sg.MLine(default_text=self.event_log, font=log_font, enable_events=True, size=(120, 10), autoscroll=True, key='-EVENT_TEXT-', reroute_stdout = True, expand_x=True, expand_y=True)]
                   ]
###########################################################################################################################
        left_col = [
            [sg.Text('Demo Programs', font='Any 20')],
            [sg.Listbox(values='', select_mode=sg.SELECT_MODE_EXTENDED, size=(40, 20), key='-DEMO LIST-')],
            [sg.Text('Filter:', tooltip=''), sg.Input(size=(25, 1), enable_events=True, key='-FILTER-', tooltip='')],
            [sg.Button('Run'), sg.Button('Copy'), sg.B('Edit')],
            [sg.Text('Find:', tooltip=''), sg.Input(size=(25, 1), enable_events=True, key='-FIND-', tooltip='')],
        ]

        right_col = [
            [sg.Text('GitHub Demo Programs', font='Any 20')],
            [sg.Listbox(values='', select_mode=sg.SELECT_MODE_EXTENDED, size=(40, 20), key='-GIT DEMO LIST-')],
            [sg.Button('Run', key='Run Git Version')],
        ]

        #test_slice_layout = [[sg.vtop(sg.Column(left_col, element_justification='c')), sg.VSeperator(), sg.vtop(sg.Column(right_col, element_justification='c'))]]
        gnb_slice_layout = [[sg.Text('Configure Setting:', font=sec_hdr_font, pad=((5,0),(12,12))),sg.Button('Setting', enable_events=True, key='-gnb_slice_configure-', pad=((5,5),(12,12))),
                             sg.Text('Start Simulation:', font=sec_hdr_font, pad=((5,0),(12,12))),sg.Button('Start', enable_events=True, key='-gnb_slice_start-', pad=((5,5),(12,12))),
                             sg.Text('Stop Simulation:', font=sec_hdr_font, pad=((5,0),(12,12))),sg.Button('Stop', enable_events=True, key='-gnb_slice_stop-', pad=((5,5),(12,12))),
                             sg.Text('Show Result:', font=sec_hdr_font, pad=((5,0),(12,12))),sg.Button('Result', enable_events=True, key='-gnb_slice_result-', pad=((5,5),(12,12))),],
                            [sg.MLine(default_text=self.pt_initial_log, font=log_font, enable_events=True, size=(120, 30), autoscroll=True, key='-gnb_slice_layout-', expand_x=True, expand_y=True)],]
        gnb_slice_layout_all = [[sg.Frame('Gnb slice simulation', gnb_slice_layout, pad=((5,5),(15,15)), expand_x=True, expand_y=True)]]

        gnb_access_layout = [[sg.Text('Configure Setting:', font=sec_hdr_font, pad=((5,0),(12,12))),sg.Button('Setting', enable_events=True, key='-gnb_access_configure-', pad=((5,5),(12,12))),
                             sg.Text('Start Simulation:', font=sec_hdr_font, pad=((5,0),(12,12))),sg.Button('Start', enable_events=True, key='-gnb_access_start-', pad=((5,5),(12,12))),
                             sg.Text('Stop Simulation:', font=sec_hdr_font, pad=((5,0),(12,12))),sg.Button('Stop', enable_events=True, key='-gnb_access_stop-', pad=((5,5),(12,12))),],
                            [sg.MLine(default_text=self.pt_initial_log, font=log_font, enable_events=True, size=(120, 30), autoscroll=True, key='-gnb_access_layout-', expand_x=True, expand_y=True)],]
        gnb_access_layout_all = [[sg.Frame('Gnb access simulation', gnb_access_layout, pad=((5,5),(15,15)), expand_x=True, expand_y=True)]]

        tab_5g_layout = [ 
                         [sg.TabGroup([[sg.Tab('Gnb Slice', gnb_slice_layout_all),
                                        #sg.Tab('Access', gnb_access_layout_all),
                                     ]],
                                    key='-tab_5g_layout-',                                    
                                    selected_title_color='purple',
                                    #tab_location='left',
                                    expand_x=True, expand_y=True,
                                    )]
                        ]
#-------------------------------------------------------------------------------------------------------------------#
        demo_layout1_hide = [
                         [sg.Button('UE', enable_events=True, key='-UE2-', pad=((10,5),(12,12))),
                          sg.Button('GNB', enable_events=True, key='-GNB2-', pad=((5,5),(12,12))),
                          sg.Button('CN', enable_events=True, key='-CN2-', pad=((5,5),(12,12)))]  
                       ]
        demo_layout1 = [
                         [sg.Canvas(key='-CANVAS-')],
                         [sg.Button('UE', enable_events=True, key='-UE1-', pad=((10,5),(12,12))),
                          sg.Button('GNB', enable_events=True, key='-GNB1-', pad=((5,5),(12,12))),
                          sg.Button('CN', enable_events=True, key='-CN1-', pad=((5,5),(12,12)))],
                          [self.Collapsible(demo_layout1_hide, '-TAB_DEMO_LY2-',  'demo', collapsed=True)]
                       ]

        LEO_satellite_layout = [
                          [sg.Frame('LEO satellite simulation', demo_layout1, pad=((5,5),(15,15)))],
                       ]
        tab_satellite_layout = [ 
                             [sg.TabGroup([[sg.Tab('LEO Satellite', LEO_satellite_layout),
                                         ]],
                                        key='-tab_satellite_layout-',                                    
                                        selected_title_color='purple',
                                        )]
                          ]
#-------------------------------------------------------------------------------------------------------------------#
        section_simulation =  [
                                [sg.TabGroup([[
                                               #sg.Tab('Satellite System', tab_satellite_layout),
                                               sg.Tab('5G System', tab_5g_layout),
                                             ]],
                                            key='-section_simulation-', 
                                            #title_color='red',
                                            selected_title_color='purple',
                                            #tab_location='left',
                                            expand_x=True, expand_y=True,
                                            )]
                              ]
#############################################################################################################################
        all_layout = [
                         [sg.Menu(menu_def, tearoff=False, pad=(50, 50))],
                         [sg.TabGroup(  [[sg.Tab('Console', section_console),
                                          sg.Tab('Simulation', section_simulation)]],
                                          selected_title_color='purple', expand_x=True, expand_y=True,
                                     )]
                     ]
        window = sg.Window('OpenSET Platform Toolkit', all_layout, 
                            auto_size_text=True,
                            right_click_menu=right_click_menu_def,
                            right_click_menu_tearoff=True,
                            grab_anywhere=True, 
                            #margins=(0,0),
                            #use_custom_titlebar=False,
                            resizable=True,
                            #no_titlebar=False,
                            return_keyboard_events=True,
                            enable_close_attempted_event=True,
                            finalize=True)
        
        #window.set_min_size(window.size)

        #window.bind("<Configure>","Configure")
        return window    

    def execute(self):

        key = sg.popup_get_text('Password: (default = 123456)', password_char='*')
        if key == 'root':                  # Remove when pasting into your program
            HashGeneratorGUI()                  # Remove when pasting into your program
            return                              # Remove when pasting into your program
        if key and PasswordMatches(key, self.PT_HASH_KEY):
            logger.info('Login SUCCESSFUL')
        else:
            sg.popup_error('Password  Failed', title='Warning', grab_anywhere=True, modal=True)  
            logger.info('Login FAILED!!')
            return

        sys_target_str = "OpenSET Toolkit version %s initialized at %s" % (self.APP_VERSION, datetime.now().strftime("%m/%d/%Y"))
        sys_comm_str = "OpenSET Toolkit target host %s, listen port %d" % (self.PT_TARGET_HOST_ADDR, self.PT_TARGET_LISTEN_PORT)
    
        logger.info(sys_target_str)
        logger.info(sys_comm_str)
        
        try:
             #initial step
             logger.info("Successfully created application objects")
        
        except RuntimeError:
            print("Error creating telecommand/telemetry objects and/or telemetry server. See log file for details")
            logger.error("Error creating application objects")
            sys.exit(2)

        self.window = self.create_window(sys_target_str, sys_comm_str)
        # --- Loop taking in user input --- #
        while True:
            self.event, self.values = self.window.read(timeout=250)
            logger.debug("App Window Read()\nEvent: %s\nValues: %s" % (self.event, self.values))            
            #if self.event in (sg.WIN_CLOSED, 'Exit') or self.event is None :
            if (self.event == sg.WINDOW_CLOSE_ATTEMPTED_EVENT or self.event == 'Exit') and sg.popup_yes_no('Do you really want to exit?', title='Confirm', grab_anywhere=True, modal=False) == 'Yes':
                if self.pt_start_subprocess is not None:
                    subprocess.call('cd %s && ./openSET -stop' % self.pt_abs_base_path, shell=True)
                    self.update_history_log()
                    if self.pt_start_subprocess.poll() is None:
                        logger.info("Killing OpenSET after subprocess poll")
                        subprocess.call('pkill -9 openSET', shell=True)

                if self.pt_window_stdout is not None:
                    logger.info("Killing Esl Process")
                    self.pt_window_stdout.terminate()  # I tried to join() afterwards and it hangs
                    t_end = time.time() + 5
                    while time.time() < t_end:
                    #for i in range(1000):
                        sg.popup_animated(sg.DEFAULT_BASE64_LOADING_GIF, no_titlebar=True, time_between_frames=100)
                    sg.popup_animated(None)

                subprocess.call('pkill -9 openSET', shell=True)                    
                break

            ######################################
            ##### test #####
            ######################################

            #if self.event == 'Configure':
                #print(self.window.size)

            #######################
            ##### MENU EVENTS #####
            #######################
            ### SYSTEM ###
            if self.event == 'OSET setting':
                path_filename = os.path.join(self.path,'oset.ini')
                self.text_editor = sg.execute_py_file("texteditor.py", parms=path_filename, cwd=self.pt_tool_path)

            if self.event == 'Logging setting':
                path_filename = os.path.join(self.path,'logging.ini')
                self.text_editor = sg.execute_py_file("texteditor.py", parms=path_filename, cwd=self.pt_tool_path)
            
            elif self.event == 'About':
                about_msg = ('The OpenSET Platform Toolkit is\n' 
                             'a PySimpleGUI based pogram that allows users to develop,\n'
                             'integrate, and run oset apps.\n\n'
                             'Version.......{}'.format(self.APP_VERSION))
                sg.popup(about_msg,
                         title='About OSET-Platform Toolkit Toolkit', 
                         grab_anywhere=True)
       
            ### DEVELOPER ###

            elif self.event == 'Create App':
                self.create_app.execute()
                self.display_event("Console->Create App")

            elif self.event == 'Download App':
                app_store = AppStore(self.config.get('APP','APP_STORE_URL'), self.config.get('PATHS','USR_APP_PATH'))
                app_store.execute()
                self.display_event("Console->Download App")
 
            elif self.event == 'Install/Uninstall':
                manage_cfs = ManageCfs(self.path, self.config.get('PATHS', 'USR_APP_PATH'), self.window)
                manage_cfs.execute()
                self.display_event("Console->Build and Install")

            elif self.event == 'Run Perf Monitor':
                subprocess.Popen("java -jar ../perf-monitor/CPM.jar",shell=True)  #TODO - Use ini file path definition
                self.display_event("Console->Run Perf Monitor")
                
            ### OPERATOR ###

            elif self.event == 'Manage Tables':
                self.ComingSoonPopup("Manage OpenSET app JSON tables")

            ### DOCUMENTS ###
            elif self.event == 'OSET Overview':
                path_filename = os.path.join(self.path, "../docs/01_OSET-Overview.docx")  #TODO - Ini file
                #url_filepath = 'file://' + path_filename
                #webbrowser.open_new(url_filepath)
                #subprocess.Popen([path_filename],shell=True) # Permision Denied
                subprocess.call(["xdg-open", path_filename]) # Not portable
                self.display_event("Console->OSET Overview")
                
            ### TUTORIALS ###
            elif self.event in self.manage_tutorials.tutorial_titles:
                tutorial_plugin_dir = os.path.join(self.path, "plugin")
                #print('tutorial_plugin_dir:%s' % tutorial_plugin_dir)
                tutorial_dir = self.manage_tutorials.tutorial_lookup[self.event].path
                #print('tutorial_dir:%s' % tutorial_dir)
                self.tutorial = sg.execute_py_file("tutorial.py", parms=tutorial_dir, cwd=tutorial_plugin_dir)
                
            ###right_click_menu###
            elif self.event == 'Files Edit':
                self.text_editor = sg.execute_py_file("texteditorplus.py", cwd=self.pt_tool_path)
                self.display_event("Console->Files Edit")

            elif self.event == 'Graph Edit':
                self.text_editor = sg.execute_py_file("graphEdit.py", cwd=self.pt_tool_path)
                self.display_event("Console->Graph Edit")
            #################################
            ##### TOP ROW BUTTON EVENTS #####
            #################################
            ##### section_console #####
            elif self.event == '-START_OSET_PT-':
                if self.pt_start_subprocess is not None:
                    #sg.popup_error("OpenSET Platform Start duplicate ", title='Warning', grab_anywhere=True, modal=False)
                    display_notification(title_err, "Duplicated Start OpenSET Platform", img_error, 100, True)
                    self.display_event('Console->OpenSET Platform Start duplicate')                  
                else:
                    subprocess.call('cd %s && rm -rf sset.log*' % self.pt_abs_log_path, shell=True)
                    self.pt_start_subprocess = subprocess.Popen('cd %s && ./openSET -nc' % self.pt_abs_base_path, shell=True, start_new_session=True)
                    while not os.path.exists(self.pt_abs_log_path + '/sset.log'):
                        #self.window['-GIF_LOAD_IMAGE-'].update_animation(sg.DEFAULT_BASE64_LOADING_GIF, time_between_frames=50) 
                        sg.popup_animated(sg.DEFAULT_BASE64_LOADING_GIF, no_titlebar=True, time_between_frames=100)
                        #time.sleep(0.1)
                    sg.popup_animated(None)      # close all Animated Popups
                    self.window['-PT_PROCESS_TEXT-'].update(self.pt_banner_log)
                    display_notification(title_ok, "Start OpenSET Platform", img_success, 100, True)
                    self.display_event('Console->OpenSET Platform Start')

            elif self.event == '-STOP_OSET_PT-':
                if self.pt_start_subprocess is not None:
                    subprocess.call('cd %s && ./openSET -stop' % self.pt_abs_base_path, shell=True)
                    self.update_history_log()
                    if self.pt_start_subprocess.poll() is None:
                        logger.info("Killing OpenSET %d after subprocess poll" % self.pt_start_subprocess.pid)
                        os.killpg(self.pt_start_subprocess.pid, signal.SIGKILL)
                        sg.popup_error("OpenSET failed to terminate.\nUse another terminal to kill the process.", title='Warning', grab_anywhere=True, modal=False)

                if self.esl_con_success_flag:
                    self.esl_con.disconnect()
                    #print('esl con result:%d' % self.esl_con.connected())
                    self.esl_con_success_flag  = False
                    if self.pt_window_stdout is not None:
                        self.pt_window_stdout.terminate()
                           
                for i in range(1000):
                    sg.popup_animated(sg.DEFAULT_BASE64_LOADING_GIF, no_titlebar=True, time_between_frames=100)
                sg.popup_animated(None)
                self.window['-PT_PROCESS_TEXT-'].update(self.pt_initial_log)
                if self.pt_start_subprocess is not None:
                    display_notification(title_ok, "Stop OpenSET Platform", img_success, 100, True)
                self.pt_start_subprocess = None
                self.pt_window_stdout = None
                self.display_event('Console->OpenSET Platform Shutdown')

            elif self.event == '-ESL_CONFIG_CMD-':
                esl_config = self.values['-ESL_CONFIG_CMD-']
                if(esl_config == self.esl_config_cmds[1]):
                    self.esl_con = ESLconnection(self.PT_TARGET_HOST_ADDR, self.PT_TARGET_LISTEN_PORT, self.PT_PASSWORD)
                    if self.esl_con.connected():
                        self.esl_con_success_flag  = True
                        self.pt_window_stdout = ConsoleStdout(self.esl_con, self.window, '-PT_PROCESS_TEXT-')
                        self.pt_window_stdout.start()
                        display_notification(title_ok, "ESL Connect", img_success, 100, True)
                    else:
                        #sg.popup_error("Esl connect fail, Please Start PT.", title='Warning', grab_anywhere=True, modal=False)
                        display_notification(title_err, "Esl connect fail, Please Start PT.", img_error, 100, True)
                    self.display_event('Console->ESL Connect')
                elif(esl_config == self.esl_config_cmds[2]):
                    if self.esl_con_success_flag:
                        self.esl_con.disconnect()
                        #print('esl con result:%d' % self.esl_con.connected())
                        self.esl_con_success_flag  = False
                        if self.pt_window_stdout is not None:
                            self.pt_window_stdout.terminate()
                            for i in range(1000):
                                sg.popup_animated(sg.DEFAULT_BASE64_LOADING_GIF, no_titlebar=True, time_between_frames=100)
                            sg.popup_animated(None)
                        display_notification(title_ok, "ESL Disonnect", img_success, 100, True)
                    self.display_event('Console->ESL Disonnect')

            elif self.event == '-SEND_CLI_TEXT-':
                if self.esl_con_success_flag:
                    query = self.values['-CLI_TEXT-'].rstrip()
                    # EXECUTE YOUR COMMAND HERE
                    if len(query):
                        logger.info('The command you entered was {}'.format(query))
                        self.command_history.append(query)
                        self.history_offset = len(self.command_history)-1
                        # manually clear input because keyboard events blocks clear
                        self.window['-CLI_TEXT-'].update('')
                        e = self.esl_con.api(query)
                        self.window['-PT_PROCESS_TEXT-'].update(e.getBody())
                        self.display_event('Console->Send CLI Command: %s' % query)
                    else:
                        sg.popup_error('Please input CLI command!', title='Warning', grab_anywhere=True, modal=False);
                else:
                    sg.popup_error('Please Connect ESL', title='Warning', grab_anywhere=True, modal=False);

            elif self.event == '-CLEAR_PT_PROCESS_TEXT-':
                if self.pt_window_stdout is not None:
                    self.pt_window_stdout.pt_console_log = ''
                self.window['-PT_PROCESS_TEXT-'].update('')
                self.display_event("Console->Cleared Platform display")

            elif self.event == '-CLEAR_EVENTS-':
                self.event_log = ""
                self.display_event("Console->Cleared Event display")
          
            elif 'Up' in self.event and len(self.command_history):
                command = self.command_history[self.history_offset]
                # decrement is not zero
                self.history_offset -= 1 * (self.history_offset > 0)
                self.window['-CLI_TEXT-'].update(command)
                #self.display_event('Console->Up CLI Command: %s' % command)
            
            elif 'Down' in self.event and len(self.command_history):
                # increment up to end of list
                self.history_offset += 1 * (self.history_offset < len(self.command_history)-1)
                command = self.command_history[self.history_offset]
                self.window['-CLI_TEXT-'].update(command)
                #self.display_event('Console->Down CLI Command: %s' % command)

            elif self.event == '-CLI_HISTROY_TEXT-':
                sg.popup_scrolled('Last 20 records:', '\n'.join(self.command_history[-20:]), title='CLI History', grab_anywhere=True, modal=False);
                self.display_event('Console->Query CLI history')

            #######################
            ##### SIMU EVENTS #####
            #######################
            ##### section_simulation #####     
            elif self.event == ('-gnb_slice_configure-'):
                path_filename = os.path.join(self.path, 'mods/gnb-slice-sim/slicesim' ,'gnb_slice_sim.yml')
                print(path_filename)
                self.text_editor = sg.execute_py_file("texteditor.py", parms=path_filename, cwd=self.pt_tool_path)
                self.display_event('Simulation->5G system->Gnb Slice setting')

            elif self.event == ('-gnb_slice_start-'):
                if self.gnb_slice_sim_subprocess is not None:
                    display_notification(title_err, "Duplicated Start Gnb slice simulation", img_error, 100, True)
                    self.display_event('Simulation->5G system->Gnb Slice start duplicate')    
                else:
                    gnb_slice_filename = os.path.join(self.path, 'mods/gnb-slice-sim')            
                    self.gnb_slice_sim_subprocess = subprocess.Popen('cd %s && python3 -m slicesim gnb_slice_sim.yml' % gnb_slice_filename,
                                                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, bufsize=1, universal_newlines=True, start_new_session=True)
                    #preexec_fn=os.setsid == start_new_session=True
                    if self.gnb_slice_sim_subprocess is not None:
                        self.gnb_slice_sim_stdout = PopenStdout(self.gnb_slice_sim_subprocess, self.window, '-gnb_slice_layout-', False)#self.main_window
                        self.gnb_slice_sim_stdout.start()
                    display_notification(title_ok, "Start Gnb slice simulation", img_success, 100, True)
                    self.display_event('Simulation->5G system->Gnb Slice start')

            elif self.event == ('-gnb_slice_stop-'):
                if (self.gnb_slice_sim_subprocess is not None):
                    while self.gnb_slice_sim_subprocess.poll() is None:
                        logger.info("Killing Gnb Slice Simulation %d after subprocess poll" % self.gnb_slice_sim_subprocess.pid)
                        os.killpg(self.gnb_slice_sim_subprocess.pid, signal.SIGKILL)
                        sg.popup_animated(sg.DEFAULT_BASE64_LOADING_GIF, no_titlebar=True, time_between_frames=100)
                    sg.popup_animated(None)
                    if self.gnb_slice_sim_stdout is not None:
                        self.gnb_slice_sim_stdout.terminate()  # I tried to join() afterwards and it hangs
                    self.window['-gnb_slice_layout-'].update(self.pt_initial_log)
                    if self.gnb_slice_sim_subprocess is not None:
                        display_notification(title_ok, "Stop Gnb slice simulation", img_success, 100, True)
                    self.gnb_slice_sim_subprocess = None
                    self.gnb_slice_sim_stdout = None
                    self.display_event('Simulation->5G system->Gnb Slice stop')
                self.window['-gnb_slice_layout-'].update(self.pt_initial_log)

            elif self.event == ('-gnb_slice_result-'):
                gnb_slice_filename = os.path.join(self.path, 'mods/gnb-slice-sim/gnb_slice_sim.png')  
                subprocess.call(["xdg-open", gnb_slice_filename]) # Not portable
                self.display_event('Simulation->5G system->Gnb Slice result')

            elif self.event.startswith('-TAB_DEMO_LY2-'):
                self.window['-TAB_DEMO_LY2-'].update(visible=not self.window['-TAB_DEMO_LY2-'].visible)
                self.window['-TAB_DEMO_LY2-'+'-BUTTON-'].update(self.window['-TAB_DEMO_LY2-'].metadata[0] if self.window['-TAB_DEMO_LY2-'].visible else self.window['-TAB_DEMO_LY2-'].metadata[1])
            
        self.shutdown()


if __name__ == '__main__':

    green = b'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'
    red = b'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'

    title_ok = "Action completed successfully"
    title_err = "Action completed Failed"
    img_error = b'iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAA3NCSVQICAjb4U/gAAAACXBIWXMAAADlAAAA5QGP5Zs8AAAAGXRFWHRTb2Z0d2FyZQB3d3cuaW5rc2NhcGUub3Jnm+48GgAAAIpQTFRF////20lt30Bg30pg4FJc409g4FBe4E9f4U9f4U9g4U9f4E9g31Bf4E9f4E9f4E9f4E9f4E9f4FFh4Vdm4lhn42Bv5GNx5W575nJ/6HqH6HyI6YCM6YGM6YGN6oaR8Kev9MPI9cbM9snO9s3R+Nfb+dzg+d/i++vt/O7v/fb3/vj5//z8//7+////KofnuQAAABF0Uk5TAAcIGBktSYSXmMHI2uPy8/XVqDFbAAAA8UlEQVQ4y4VT15LCMBBTQkgPYem9d9D//x4P2I7vILN68kj2WtsAhyDO8rKuyzyLA3wjSnvi0Eujf3KY9OUP+kno651CvlB0Gr1byQ9UXff+py5SmRhhIS0oPj4SaUUCAJHxP9+tLb/ezU0uEYDUsCc+l5/T8smTIVMgsPXZkvepiMj0Tm5txQLENu7gSF7HIuMreRxYNkbmHI0u5Hk4PJOXkSMz5I3nyY08HMjbpOFylF5WswdJPmYeVaL28968yNfGZ2r9gvqFalJNUy2UWmq1Wa7di/3Kxl3tF1671YHRR04dWn3s9cXRV09f3vb1fwPD7z9j1WgeRgAAAABJRU5ErkJggg=='
    img_success = b'iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAA3NCSVQICAjb4U/gAAAACXBIWXMAAAEKAAABCgEWpLzLAAAAGXRFWHRTb2Z0d2FyZQB3d3cuaW5rc2NhcGUub3Jnm+48GgAAAHJQTFRF////ZsxmbbZJYL9gZrtVar9VZsJcbMRYaMZVasFYaL9XbMFbasRZaMFZacRXa8NYasFaasJaasFZasJaasNZasNYasJYasJZasJZasJZasJZasJZasJYasJZasJZasJZasJZasJaasJZasJZasJZasJZ2IAizQAAACV0Uk5TAAUHCA8YGRobHSwtPEJJUVtghJeYrbDByNjZ2tvj6vLz9fb3/CyrN0oAAADnSURBVDjLjZPbWoUgFIQnbNPBIgNKiwwo5v1fsQvMvUXI5oqPf4DFOgCrhLKjC8GNVgnsJY3nKm9kgTsduVHU3SU/TdxpOp15P7OiuV/PVzk5L3d0ExuachyaTWkAkLFtiBKAqZHPh/yuAYSv8R7XE0l6AVXnwBNJUsE2+GMOzWL8k3OEW7a/q5wOIS9e7t5qnGExvF5Bvlc4w/LEM4Abt+d0S5BpAHD7seMcf7+ZHfclp10TlYZc2y2nOqc6OwruxUWx0rDjNJtyp6HkUW4bJn0VWdf/a7nDpj1u++PBOR694+Ftj/8PKNdnDLn/V8YAAAAASUVORK5CYII='

    close64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsSAAALEgHS3X78AAAE30lEQVRIiZ2VXYgdRRqGn6+quvucM/85iRoTNevMBJFEWY0GFQTBC1HBlaz/jMpoFFfXBdmFvdiLvRIEFRHFGBXMjUQhF/6Bol6sSNaIruCNir/R/Dlx5iRzck736e6qby/6JDlx9CIWFN10Ue/7vW+9X7XcDn8bryWPL2vERkNQQPj9Q72K7F3s7Hxb9bZ98L0bj91jt1y23kxNTxIEGUQ/aTYR6WW9cud/Prx01zf7/7FP5EHXHG7Y6bVTpBPLMSegCWKEEMKvkihgjEWDP+FbEjxTa1bjv9l/CsIKF3ypHhUDSFGACCKC956iKKjV6/hfkCjgUNK0TW1oCA3h+EJk8UUBYFCsQaSyRajArUWLnEONcTrT68nTLtZaEKmmMTiUlsREGy9HO0dgcL1y6lgtZrAsEYFexhwxq2buYfru+1mcOo+828UYg4rgUH7OSkY3zbDq1lkaV1yFP9TqEyy18jiBCMF7DjYmOOu+hxifnCSKItZuvp/F6fPJ05TEwE+dHhN33MfpGy4iFAVjf7qF8etvBV9y1IilBApGIMt6TExOM372JKqKqhLFMdOz93Jk6jx+bHVoztzLyj9eiHqP2Gq7O3UlGAuq1RwYDlUwhoChMdSAz3ZxaEeD8T/fBggaAnGtxpqZWdKFBSbOPLMCCQGJItJPdrHw4lOYRgNsBM6dSCDGErIuodtGkhoyPEr68U5svcbI1ZsQY0CV2vAw9ZGRKjEiSBTR/fQjDm9/AddcjqoSul182kYHVDhJauRffUH7wD7ilatxzVOwI6PM7XiJLO2x4rob0CgGVTSEKigidD94j/ltW9Dg0b0/4BfmyQ8ewKUdWLZ6wCIB9SXFXJvQ+hLkc6QeEznHf199jY1rpjh1w0ZUFTGm7z18/tSj2Hffor5shKLdhhJCADMcw7IlKRIkAqkJRIa4LPl6d5c/PPJkBd5vpArcArD+ue101l1Md08bFxuIBUlOyOUggUIAVIl94Kv5wKqtz7L+7r/0bRHEmApcFbwnHhljw6tv0b3kEtK5gDWmj/GbfQAWZbdaztjyPOfP3oN6D8GDCO133uDAvx9CyxKsRX1JMjbBBa+8Rnbl5RSpR35RfXUGfVLnYGFBcTfdwLo77yLkPYy14CLa773JngfuoNy7QOh2WPnw09WVkufUm8s598G/s+eT9wmBJZ1m+sVTFNBc4Wi8vJ3v//kAJk7AOhbf3MGezTfjWwuYCcv8s1s58K+/okWOxDGdjz5g7+YZtKRSoL+igCp5FKVntGk48sTTzDWb1C+4mB833wgETD2CELBjEfNbtyAjo4xdcz27N11L6B5GGoZQhN+26KiSoII9LebnJx9BkggzNIQkyfEdItiRQGvbM7S2bQHJMGN1NO8ds2dQhBORYBCjAFEE1kFSw0QxuAiTJCAGce64vz4gviTkOTJcErIMMRbyDIxg7bHTFnc47clcmpdj43VkeBRJEkytgdTqSL2OiRMkSRDroH9t4EtCUaBZhmYpIUurZ9pFfVnuX+w62xfjeq3D3/6vbifXrT1XkzgWdREmipA4RlwMUYRY21cg/X+lJ5gSbIHGOVovCHmOCSX7DrbMx599icIhVI2cA5c5mC1gbGnITm4oqAOr0PoOXs9g51HAGiITyCDByXDp4KuiaoESmP8/YC0Y5GajmEsAAAAASUVORK5CYII='
    checkmark64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsSAAALEgHS3X78AAAD2ElEQVRIibWVe4hUVRzHP+feuTOz82gfrqwu5gM0zTQpS1yfoGQPyc3IjOwlYRElPcDICEP6Q6i/IvojCCIpTA0tSlSqJSvbTLZE0dbWxz4nXWdnxrk7M/feOY/+sHyEa7pt57/z45zP53x/B84R/B/jzeNLgeXADDHE4IeBV2Ihpo6LK1rzNqEhAW9oGw18UhelYeWUGHFL0tx5lqPZyBAI3mi9o8YRm16cWTlsVFLQVwjY2+4SSI2W+j8KXj+ybmwytO69xjo7Wyqzr8sldbaE60ksIdBlhTVo+KuHXppY5azftKzeNsbQkurntOuTKQQobYhFbCgPNsGaA5NDWm94ZlYV7fmAX3pcenIlTucDAqlJRENUxcJQLgwiwfMtYcpq4503JMJjq8M0d+XpyBRJnfUpBpJwyKYqFqbCcSCQg0gQyCeq4qHp90yr5Pd0kY6+ImnXJ1CaeDhEdSJCTSJKzLEHLXhu4oQEuWKZ79uzZAoX2hKPhOn+I6DtuEdfLriC4NE9L4CYhzEP8dH84Hz9kT0NBHLqvMlJmo5nyBQDylITj4RwM5rmw70orcEA0AL8Q/DgN8OBr/DltL8q64G1F52+obomwr6US7boE0hNhRPiVIdHx7H+EvA2sJ0tC3/+e8uFS27c/SS+7ElGrGkbnp5EfV0UArmGxt0Lzq/x5YzKWocz/T4FXyGEINvj0XE410QgJ7Fl4dqL4ecS3PVlJYgdllKzx04ZxqolY8h4mkm315JPl+z+nP8Bd++4hZ2LM/hyuokLCr7Eti28TJnOA5ndGLOUnYtLl+u2YMHnJ4BxY2bWsWj2SA72eoBBG4PnBvy2qwvpq81gVjhJp1Q7q9axLIFVMqSaz3ytfLWEpsbLwgFs6pc1o/R9+e7+eK9joSMWvjR4gSLA4FSGKLS7UyirUmRkbJFTG0VI6N17+oR0/bl8d/+A8HMJAG7bPB7BTmGL8TVz64mMiKGNQSuN0hqvq59CS59Kzq2zo8MrcH/s1V6qMIf9y5uvBL8gALj54xpgG5aYH589klB9BdoYjDY0XJ9k9HURPj2aRZ/ycL/tfouDK17+N/ilAoAbP6wAsRGLB8INI7BGJUAYLGEhLAtLCApfnDymc95NtD4eDMC8ZNiXzNKfSdLbt5K8N6o68nNMwoHqKCAwlkVwKI06ln2MtpWtVwMHBnjspHyNQO1Xe7pRbTmUEchCGbk/laKsdl0tfGBB51OKQM0hUD/ppk7kkTTy11NQku/TuUpdi+DKn/7wdyuAHzDcii0Uykwg/ezJoRMAVL9TCWwFjpJdvfpa4AB/Akx4zQw8GDagAAAAAElFTkSuQmCC'
    cookbook64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABQAAAAYCAIAAAB1KUohAAAACXBIWXMAAAsSAAALEgHS3X78AAADMUlEQVQ4jY1UQUgCWxS9782M5hQzle3NTRGEQdBipCKKECoEg4JaJAqCmyBsYauEaNOqjQtXLSKINuFiKMiwIMtF1iLK2kWRZLUoDRxo5s37ixfSt4+/u7xzz51z7r3nwc7Ojt1uBwCEEPwtWKXL5eIvLy+HhoYIIYIgCIJAKa0Do5RyHPfy8nJycnJ1dcUjhJxOZygUOj09zefzFoulDp5SihCKRqPLy8vJZBI4jgOAo6Mjj8cDABjj/6WdTqdDoRAAfJeyFn8MQohhGADAY4xFUSyVSpIkAYBpmgih+soRQmxm2GazbW5u7u7ujoyMKIrCmP+ePMdxv9nhSqXi8/lmZmb29vay2Syrs1gs8EM/QogQQgipBWOMOzs7397eWlpabDYbAMiyHAwGu7u7mQTWzu/3R6PRxsZG+HERvNVqjcVix8fHfX19Nzc3T09PHo+HUjo1NVUulx8fHwFgbW0tEolQSguFwtbWVpU/rlQqs7Ozc3NzqqrmcjmXy9Xe3m61WgcGBubn5wGgo6NjYWEBAEql0t3dHQBUx8ljjNva2orFYnNzM8/zBwcHFoslGo329/cXCgUA6OnpwRh/fHwsLS3lcjm2qm9wQ0NDPB7f398fHBx8eHjIZrOqqhaLRUmSwuFwPB53OBw+ny+dTn9+ftYujed5AEilUhMTE9U9saTX66WUJhKJmv0dHh4Gg0FgF4YxJoQwANNjGIaiKLFYbHp62ul0Li4umqb5H5crSVIymQwEAolEwu12s6SiKNfX15OTkwDgcDguLi4ikUgVUv0zCIJgs9lUVWWlrP3q6qrf72dfAaCrq2tjY0OW5RowTynVNM1qteq6XqW9srJiGAZCSNd1hNDt7W04HGZm+NeFiaKYTCa3t7fHx8fdbjez+9fXV7UR87Cu66Zp1oI1TQsEAl6vN51Os9smhCCEfpbWmMw0TZbBpmm+v7+3traWy2VKKdP825I/M7Isi6IIAFxTU9P6+nomk+nt7X19fX1+fsYY1/ez0+k8Pz+/v7/nMMblcnl4eDifz5+dnWmaVgfGolQq2e32sbGx7wcok8mMjo7C396wVCpFKSWE/ANWXYLwO0+V8wAAAABJRU5ErkJggg=='
    download64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsSAAALEgHS3X78AAAEl0lEQVRIia2WS2hdZRDHf/Odc+7NvbZpYky1Gk2rEbRa26pt8UWLitYHIqigCD426kIQxCfWhYu68U0RBBERXCq+WrXoorYIFWpbEY0lpk0NTWKtuUlzn+d834yLm5smqQspzurMcM785j+c+eYTTtUcESAzvhIAm/+azHacA4FodszAVNFTrUPm+Q6iecmUyJkEna5OiDCCXCtvJ2cnV+Ep+9R7/VIfQhmeryKeySywok+SSMMKMwqAihDXPIcPDDMURUQCgiPWjJCck6x87ZXXV3cXu3XTO5tkYOvAHbnIfZipTpghLdAMIEngi1cXvtlzwfrHpG0x5ismzsvo0E9D9z7z++M799s2EcSm67OUkAs5cpbzkkoMtPtAAzdXQ9zqjHkt1Ol5SHofx0KWYRUxrdiS3FlLtzz51wd7+v2OQl7qHnPtorUXS3ZxPRUKUT5x4mTDWu559LbCNS+9X9v025Duc4KoYdMAA7A4Mk92EMp/JFIZwR/rx9dL1teVdC2/Qe8yzQg+pS0JvLUzx3hjioPVQamGGlcu47KNq6qrPj+fsd+GeAEYA2SmRQiCNSJKP1Ad3IVaG0nnlWRxKqkkVlYxJxGZwhmFIo34U/fh0Hv4v6YYrY+ihYtkorDUNj+298GPvzv6ZRrkMzA/oyCXh9rEMOOHfiLfcx+5zhXkOnppswxEpJHVxdTjs0CycDHy9XcMlwc5a0E3EoTconOls/dyBsb6lYRLY4m/9T6blDgi8oHw3rPx83fesubl4oVPWFvXBUKoQzqB92Xitpite77n/k/epaN7AZO1CTIROtZ14fJC6ccS9ndGUhRLK0Eum1h2YGpH5eFfD47sjluzcFo+f+vp655F03alNhZhASMjloA1qtzedzab125kiw2QLhHaQ0zIFM2MztUdkBcqx1Lp+0o59NGRP49OVQs0Z3d6nEyMUMP8OGgVtAJaA19CagP4xn4e6DPuPhox1V9HTRFr/h9mRmWkwbJtGSsHK4xXq4cQGQDCDABM0ClEy6DlJiA9DLV90BgktirFzhrPXX0mT6Y9lAaqkAhRItRKGT3bjetTYd2aYM7JYcwm5wwaAP44hDyQYukokg5jliICZoFIoNjZ4Ol1HdhueOPgCLlFjt7twvo63HwztGuipml20lEBBlrGfBXzR5BsDGjOPBrAAkJKRKBwuuepNUXyP5/HN7tKXFGvcuMGY/3qhAO/NLCTJ7kFmIT0OPgjmAhiYKYIASFgGoCUyAILu+o8ckng0jSwsF1YuzxP0hYwm3tizwIIpKPQOIY4BXUYCiiYYWSIKYYHMoRAV1fKTddFxJKQOA/mmW9zFWRjoCmYw6R1lrcg2kxgAfCIeRxKMa+YBSw0Vc7fOScAZuAnMXWYE8yaIUFBDFSbS8sCgscsayZWD3jMAmhT7b8CnDPIeZw6RGTOLmwWFRALMA3BZvkamoBcwM3Zh7MA9Yb5I3v/YKoKTlr9sROKZVrlTGDWsylmkMTGxCQ4h0ObGaT1aRJzHsbtwJJmWSet0/9kIpB69gPbgersJA4oMm/pn6JlQI1/uWX87/YP06p9rkZQnAYAAAAASUVORK5CYII='
    github64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAMAAADXqc3KAAAABGdBTUEAALGPC/xhBQAAAwBQTFRFAAAADAwMDQ0NAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAhyjGAAAAQB0Uk5T////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////AFP3ByUAAAAJcEhZcwAADdUAAA3VAT3WWPEAAAAYdEVYdFNvZnR3YXJlAHBhaW50Lm5ldCA0LjEuMWMqnEsAAABzSURBVChTbYxRFoAgDMPQ+98Z1zbIeJqPbU3RMRfDECqyGpjMg6ivT6NBbKTw5WySq0jKt/sHrXiJ8PwpAAVIgQGkwABSYAApMIAUGEAalFmK9UJ24dC1i7qdj6IO5F+xnxfLu0jS0c7kqxd3Dk+JY8/5AKFrLuM7mfCAAAAAAElFTkSuQmCC'
    run64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAMAAADXqc3KAAAABGdBTUEAALGPC/xhBQAAAwBQTFRFAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAszD0iAAAAQB0Uk5T////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////AFP3ByUAAAAJcEhZcwAADdUAAA3VAT3WWPEAAAAYdEVYdFNvZnR3YXJlAHBhaW50Lm5ldCA0LjEuMWMqnEsAAABqSURBVChTpY5JDsAwCMTy/09TMGvFpVF9aAZPRHpkcXC7OIodPg0uCjPq+MwCrWRGKkiIvLyTqzw3aqoI73eqUNAoXBXlg4zudxF+NONfPIVvbSZPgww5oW0Vz8T4Lgbt/xbjia+rahR5AEYEg4vdzh2JAAAAAElFTkSuQmCC'
    storage64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAMAAADXqc3KAAAABGdBTUEAALGPC/xhBQAAAwBQTFRFAAAABwcHDQ0NDg4ODw8PFxcXGRkZGhoaGxsbHh4eIyMjJSUlJiYmJycnKCgoMTExMjIyNTU1NjY2Nzc3AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAouNksgAAAQB0Uk5T////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////AFP3ByUAAAAJcEhZcwAADdQAAA3UAe+RuhUAAAAYdEVYdFNvZnR3YXJlAHBhaW50Lm5ldCA0LjEuMWMqnEsAAAC5SURBVChTfZLbDsMgDEPpbb3TDv7/W7PYuAztYUeqhO2QAGowkXIMIeYkaSU4QsNBi4GcyhNINpTglmq4GWSphvy/ldkuLXZ4HmAxy3NmFJaA4guKGCwsjClfV05+fWdhYBtFw+amB292aygW3M7fsPTwjmadZkCvHEtWaAYTViBqVwgTA3tJVnB6D/xhaimItDhjMBvlhtFsaIafnEtOaAY/twAw/eslK70CbX8obUvgJNw9Jv0+Zh8D4s5+VAm/LwAAAABJRU5ErkJggg=='

    app = App('oset.ini')
    app.execute()    
    logger.info("Exiting app")