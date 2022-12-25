#!/usr/bin/env python

import sys
import time
import os
import requests
import json
import configparser
from datetime import datetime

import logging
logger = logging.getLogger(__name__)

if __name__ == '__main__':
    from utils    import compress_abs_path
else:
    from .utils    import compress_abs_path

import PySimpleGUI as sg


###############################################################################

class GitHubAppProject():
    '''
    Manage the interface to OSET apps in github repos  
    '''
    def __init__(self, git_url, usr_app_path):
        """
        usr_app_path is an absolute path to where git repos should be cloned into
        """
        self.usr_clone_path = usr_app_path
        self.git_url  = git_url
        self.app_repo = None
        self.app_dict = {}
         
         
    def create_dict(self):
        """
        Queries git URL to a list of apps. A dictionary is created using app
        names as the keys. This function is not part of the constructor 
        to allow the caller to interact with the user in the event that the
        URL can't be accessed.
        """
        ret_status = False
        try:
            self.app_repo = requests.get(self.git_url)
            if self.app_repo.status_code == 200:
                app_repo_list = self.app_repo.json() 
                # Create a dictionary with app names as the key
                for repo in app_repo_list:                
                    print(repo['name'])
                    print(repo['git_url'])
                    self.app_dict[repo['name']] = repo
                #print(self.app_dict['kit_ci'])
                ret_status = True
        except ConnectionError:
            pass
            
        return ret_status
        

    def clone(self, app_name):
        """
        """
        if app_name in self.app_dict:
            saved_cwd = os.getcwd()
            os.chdir(self. usr_clone_path)
            clone_url = self.app_dict[app_name]["clone_url"]
            print("Cloning " + clone_url)
            os.system("git clone {}".format(self.app_dict[app_name]["clone_url"]))
            os.chdir(saved_cwd)

    def get_descr(self, app_name):
        """
        """
        descr = ''
        if app_name in self.app_dict:
            descr = self.app_dict[app_name]['description']
        return descr


###############################################################################

class AppSpec():
    '''
    The access methods are defined according to the activities a developer
    needs to do to integrate an app.
    '''
    def __init__(self, app_path, app_name):
        self.app_path  = app_path
        self.app_name  = app_name
        self.json_file = os.path.join(app_path, app_name+'.json')  
        #print(self.json_file)
        
        self.valid = False
        self.json  = None
        self.cfs   = None
        if self.read_json_file():
            self.valid = True

    def read_json_file(self):
        try:
            f = open(self.json_file)
            self.json = json.load(f)
            f.close()
            #print(str(self.json))
        except:
            sg.popup("Error loading JSON spec file %s" % self.json_file, title='Error', grab_anywhere=True, modal=False)
            return False
        
        try:
            self.others = self.json['app']['others']
        except:
            sg.popup("The JSON spec file %s does not contain the required 'others' object" % self.json_file, title='Error', grab_anywhere=True, modal=False)
            return False
        return True
        
    def get_targets_cmake_files(self):
        """
        The targets.cmake file needs
           1. The app's object file name for the 'APPLIST'
           2. The names of all the tables that need to be copied from the app's tables directory into
              the cfsat_defs directory 
        """
        files = {}
        files['type'] = self.others['type']
        files['obj']   = self.others['obj']
        return files

###############################################################################

class ManageUsrApps():
    """
    Discover what user apps exists (each app in separate directory) and
    create a 'database' of app specs that can be used by the user to integrate
    an apps into their cFS.
    """
    def __init__(self, usr_app_path):

        self.path = usr_app_path
        self.app_specs = {}
        
        usr_app_list = os.listdir(usr_app_path)
        usr_app_list.sort()
        # Assumes app directory name equals app name
        for app_name in usr_app_list:
            #print("User app folder/name: " + app_name)
            #todo: AppSpec constructor could raise exception if JSON doesn't exist or is malformed
            app_path = os.path.join(usr_app_path, app_name)
            if os.path.isdir(os.path.join(usr_app_path, app_name)):
                app_spec = AppSpec(app_path, app_name)
                if app_spec.valid:
                    self.app_specs[app_name] = app_spec        
        
        #print("User app specs: " + str(self.app_specs))

    def get_app_specs(self):
        return self.app_specs

    def get_app_spec(self, app_name):
        return self.app_specs[app_name]
            
              
###############################################################################

class AppStore():
    """
    Manage the user interface for downloading apps from github and cloning
    them into the user's app directory. 
    """
    def __init__(self, git_url, usr_app_path):

        self.usr_app_path = usr_app_path
        self.git_app_repo = GitHubAppProject(git_url, usr_app_path)
        self.window  = None

        
    def create_window(self):
        """
        """
        hdr_label_font = ('Arial bold',12)
        hdr_value_font = ('Arial',12)
        app_layout = []
        for app in self.git_app_repo.app_dict.keys():
            print(app)
            app_layout.append([sg.Radio(app, "APPS", default=False, font=hdr_label_font, size=(10,0), key='-%s-'%app),  
                               sg.Text(self.git_app_repo.get_descr(app), font=hdr_value_font, size=(100,1))])
                
        layout = [
                  [sg.Text("Select an app to download then follow the steps in 'Add App to OSET'. See 'Add App' tutorial if you are unfamiliar with the steps.\n", font=hdr_value_font)],
                  app_layout, 
                  [sg.Button('Download', font=hdr_label_font), sg.Button('Cancel', font=hdr_label_font)]
                 ]

        window = sg.Window('Download App', layout, modal=False)
        return window


    def gui(self):
        """
        """        
        self.window = self.create_window() 
        
        while True: # Event Loop
            
            self.event, self.values = self.window.read()

            if self.event in (sg.WIN_CLOSED, 'Cancel') or self.event is None:       
                break
            
            if self.event == 'Download':
                for app in self.git_app_repo.app_dict.keys():
                    if self.values["-%s-"%app] == True:
                        self.git_app_repo.clone(app)

                break
                
        self.window.close()

    def execute(self):
        """
        """        
        if self.git_app_repo.create_dict():
            self.gui()
        else:
            sg.popup('Error accessing the git url %d. Check your network connection and the git URL'%self.git_app_repo.git_url, 'Error')


###############################################################################

if __name__ == '__main__':

    config = configparser.ConfigParser()
    config.read('../oset.ini')

    git_url = config.get('APP','APP_STORE_URL')
    usr_app_path = compress_abs_path(os.path.join(os.getcwd(),'..', config.get('PATHS', 'USR_APP_PATH'))) 

    #app_store = AppStore(git_url, usr_app_path)
    #app_store.execute()
    
    
    manage_usr_apps = ManageUsrApps(usr_app_path)
    
    berry_imu = manage_usr_apps.get_app_spec('berry_imu')
    print(berry_imu.get_targets_cmake_files())
    print(berry_imu.get_startup_scr_entry())
    
    gpio_demo = manage_usr_apps.get_app_spec('gpio_demo')
    print(gpio_demo.get_targets_cmake_files())
    print(gpio_demo.get_startup_scr_entry())
    
    

