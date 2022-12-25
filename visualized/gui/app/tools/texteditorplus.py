#!/usr/bin/env python
VERSION = '0.0.1'
VERSION_DATE = '2022-07-20'

import PySimpleGUI as sg 
from tkinter import font as tkfont
from datetime import datetime
import sys
import os.path

# `application_active` flag used when updating the window after the first read, and after a theme change
# this is currently used as a work-around for the `finalize=True` bug, and can be
# removed when that issue is fixed
application_active = False 
global_floder = ''


##-----CREATE CUSTOM RE-DIRECT STDOUT OBJECT-------------##

class RedirectText:
    def __init__(self, window):
        ''' constructor '''
        self.window = window
        self.saveout = sys.stdout

    def write(self, string):
        self.window['_OUT_'].Widget.insert("end", string)

    def flush(self):
        sys.stdout = self.saveout 
        sys.stdout.flush()

##-----SETUP DEFAULT USER SETTINGS-----------------------##

save_user_settings = False
# if flag is `False` the settings are saved in active session only with a `dict`
# if flag is `True` the settings are saved in a local directory in a shelve file
if save_user_settings:
    import shelve
    settings = shelve.open('app_settings')
else:
    settings = {}

# check to see if settings dict/shelf contains default values, create if not
if len(settings.keys()) == 0:
    settings['theme'] = 'SystemDefault'
    #settings['themes'] = sg.list_of_look_and_feel_values()
    settings['themes'] = ['SystemDefault','Black', 'BlueMono', 'BluePurple', 'BrightColors', 'BrownBlue', 'Dark', 'Dark2', 'DarkAmber', 'Default', 'Default1', 'DefaultNoMoreNagging']
    settings['font'] = ('Consolas', 12)
    settings['tabsize'] = 4
    settings['filename'] = None
    settings['body'] = ''
    settings['info'] = '> New File <'
    settings['out'] = ''

# default theme or user saved theme
sg.change_look_and_feel(settings['theme'])

# string to output initial start settings
outstring = "STARTUP SETTINGS:\n"+"-"*40+"\nTheme"+"."*10+" {}\nTab size"+"."*7+" {}\nFont"+"."*11+" {} {}\nOpen file"+"."*6+" {}\n\n"
settings.update(out = outstring.format(settings['theme'], settings['tabsize'], settings['font'][0], settings['font'][1], settings['filename']))

def close_settings():
    ''' Close the the shelve file upon exit '''
    settings.update(filename=None, body='', out='', info='> New File <')
    if save_user_settings:
        settings.close()

##----SETUP tree WINDOW-----------------------------------##
folder_icon = b'iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAACXBIWXMAAAsSAAALEgHS3X78AAABnUlEQVQ4y8WSv2rUQRSFv7vZgJFFsQg2EkWb4AvEJ8hqKVilSmFn3iNvIAp21oIW9haihBRKiqwElMVsIJjNrprsOr/5dyzml3UhEQIWHhjmcpn7zblw4B9lJ8Xag9mlmQb3AJzX3tOX8Tngzg349q7t5xcfzpKGhOFHnjx+9qLTzW8wsmFTL2Gzk7Y2O/k9kCbtwUZbV+Zvo8Md3PALrjoiqsKSR9ljpAJpwOsNtlfXfRvoNU8Arr/NsVo0ry5z4dZN5hoGqEzYDChBOoKwS/vSq0XW3y5NAI/uN1cvLqzQur4MCpBGEEd1PQDfQ74HYR+LfeQOAOYAmgAmbly+dgfid5CHPIKqC74L8RDyGPIYy7+QQjFWa7ICsQ8SpB/IfcJSDVMAJUwJkYDMNOEPIBxA/gnuMyYPijXAI3lMse7FGnIKsIuqrxgRSeXOoYZUCI8pIKW/OHA7kD2YYcpAKgM5ABXk4qSsdJaDOMCsgTIYAlL5TQFTyUIZDmev0N/bnwqnylEBQS45UKnHx/lUlFvA3fo+jwR8ALb47/oNma38cuqiJ9AAAAAASUVORK5CYII='
file_icon = b'iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAACXBIWXMAAAsSAAALEgHS3X78AAABU0lEQVQ4y52TzStEURiHn/ecc6XG54JSdlMkNhYWsiILS0lsJaUsLW2Mv8CfIDtr2VtbY4GUEvmIZnKbZsY977Uwt2HcyW1+dTZvt6fn9557BGB+aaNQKBR2ifkbgWR+cX13ubO1svz++niVTA1ArDHDg91UahHFsMxbKWycYsjze4muTsP64vT43v7hSf/A0FgdjQPQWAmco68nB+T+SFSqNUQgcIbN1bn8Z3RwvL22MAvcu8TACFgrpMVZ4aUYcn77BMDkxGgemAGOHIBXxRjBWZMKoCPA2h6qEUSRR2MF6GxUUMUaIUgBCNTnAcm3H2G5YQfgvccYIXAtDH7FoKq/AaqKlbrBj2trFVXfBPAea4SOIIsBeN9kkCwxsNkAqRWy7+B7Z00G3xVc2wZeMSI4S7sVYkSk5Z/4PyBWROqvox3A28PN2cjUwinQC9QyckKALxj4kv2auK0xAAAAAElFTkSuQmCC'

treedata = sg.TreeData()

def add_files_in_folder(parent, dirname, treedata):
    files = os.listdir(dirname)
    for f in files:
        fullname = os.path.join(dirname, f)
        if os.path.isdir(fullname):            # if it's a folder, add folder and recurse
            treedata.Insert(parent, fullname, f, values=[], icon=folder_icon)
            add_files_in_folder(fullname, fullname, treedata)
        else:
            treedata.Insert(parent, fullname, f, values=[os.stat(fullname).st_size], icon=file_icon)


##----SETUP GUI WINDOW-----------------------------------##

def main_window(settings):
    ''' Create the main window; also called when the application theme is changed '''
    elem_width= 120 # adjust default width
    right_click_menu_def = [[],['File',['New','Open'],'---', 'Save','Save As', '---', 'Copy','Paste', 'Select All','---','Exit']]
    menu_layout = [
        ['File',['Mod Load','New File','Open File','Save','Save As','---','Exit']],
        ['Edit',['!Undo','---','!Cut','Copy','Paste', 'Select All' ,'---','!Find...','!Replace...','---','Date/Time']],
        ['Format',['Theme', settings['themes'],'Font','Tab Size','Show Settings']],
        ['Run',['Run Module']],
        ['Help',['View Help','---','About Me']]]

    colleft = sg.Column([ 
                        [sg.Tree(data=treedata,
                               headings=['Size', ],
                               auto_size_columns=True,
                               select_mode=sg.TABLE_SELECT_MODE_EXTENDED,
                               num_rows=20,
                               col0_width=10,
                               key='-TREE_LIST-',
                               show_expanded=False,
                               enable_events=True,
                               expand_x=True,
                               expand_y=True,
                               ), ]], expand_x=True, expand_y=True,)
    colright = sg.Column([ 
                          [sg.Multiline(default_text=settings['body'], font=settings['font'], key='_BODY_', size=(elem_width-17,25), expand_x=True, expand_y=True,), ]], expand_x=True, expand_y=True,)
    
    colup = sg.Col([[sg.Pane([colleft, colright], orientation='h', expand_x=True, expand_y=True,), ]], expand_x=True, expand_y=True,)
   
    coldown = sg.Column([[sg.Multiline(default_text=settings['out'], font=settings['font'], key='_OUT_', autoscroll=True, size=(elem_width,8), expand_x=True, expand_y=True,)]], expand_x=True, expand_y=True,)         

    window_layout = [
        [sg.Menu(menu_layout)],
        [sg.Text(settings['info'], key='_INFO_', font=('Consolas',11), size=(elem_width,1))],
        [sg.Pane([colup, coldown], orientation='v', expand_x=True, expand_y=True,)]]

    window = sg.Window('OpenSET Text Editor', window_layout,
                                           right_click_menu=right_click_menu_def,
                                           right_click_menu_tearoff=True,
                                           resizable=True,
                                           margins=(0,0), return_keyboard_events=True)
    return window


##----FILE MENU FUNCTIONS--------------------------------##
def mod_file(window):
    newtreedata = sg.TreeData()
    window['_BODY_'].update('')
    window['-TREE_LIST-'].update(newtreedata)
    try: # 'OUT OF INDEX' error in trinket if 'CANCEL' button is pressed
        modfolder = sg.popup_get_folder('Mod Folder Name:', title='Open', no_window=True)
    except:
        return
    #print(modfolder)
    if modfolder not in (None,''):
        add_files_in_folder('', modfolder, newtreedata)
        #print(treedata)
        #filelist = os.listdir(modfolder)
        #fnames = [f for f in filelist if os.path.isfile(os.path.join(modfolder, f))] #and f.lower().endswith((".png", ".jpg", "jpeg", ".tiff", ".bmp"))]
        #window['-FILE_LIST-'].update(fnames)
        window['-TREE_LIST-'].update(newtreedata)
        return modfolder
        
def choose_file(window, folder):
    if folder not in (None,''):
        #filename = os.path.join(folder, values['-FILE_LIST-'][0])
        filename = os.path.join(folder, values['-TREE_LIST-'][0])
        if filename not in (None,'') and os.path.isfile(filename):
            if filename.lower().endswith((".c", ".cc", ".cpp", ".txt", ".py", ".cmake", ".yml", ".ini", ".json", ".md", ".xml")):
                with open(filename,'r') as f:
                    file_text = f.read()
            else:
                with open(filename,'rb') as f:
                    file_text = f.read()            
            window['_BODY_'].update(value=file_text)
            window['_INFO_'].update(value=filename.replace('/',' > '))
            settings.update(filename=filename, body=file_text, info=filename.replace('/',' > '))

def new_file(window): # CTRL+N shortcut key
    ''' Setup a new session by clearing application variables and the body '''
    window['_BODY_'].update(value='')
    window['_INFO_'].update(value='> New File <')
    settings.update(filename=None, body='', info='> New File <')

def open_file(window): # CTRL+O shortcut key
    ''' Open a local file in the editor '''
    try: # 'OUT OF INDEX' error in trinket if 'CANCEL' button is pressed
        filename = sg.popup_get_file('File Name:', title='Open', no_window=True)
    except:
        return
    if filename not in (None,''):
        with open(filename,'r') as f:
            file_text = f.read()
        window['_BODY_'].update(value=file_text)
        window['_INFO_'].update(value=filename.replace('/',' > '))
        settings.update(filename=filename, body=file_text, info=filename.replace('/',' > '))

def save_file(window, values): # CTRL+S shortcut key
    ''' Save active file. If new, then calls the `save_file_as` function '''
    filename = settings.get('filename')
    if filename not in (None,''):
        with open(filename,'w') as f:
            f.write(values['_BODY_'])
        window['_INFO_'](value=filename.replace('/',' > '))
        settings.update(filename=filename, info=filename.replace('/',' > '))
    else:
        save_file_as(window, values)

def save_file_as(window, values):
    ''' Save the active file as another file, also called for new files '''
    try: # 'OUT OF INDEX' error in Trinket if 'CANCEL' button is clicked
        filename = sg.popup_get_file('Save File', save_as=True, no_window=True)
    except:
        return
    if filename not in (None,''):
        with open(filename,'w') as f:
            f.write(values['_BODY_'])
        window['_INFO_'](value=filename.replace('/',' > '))
        settings.update(filename=filename, info=filename.replace('/',' > '))


##----EDIT MENU FUNCTIONS--------------------------------##

def undo(): # CTRL+Z shortcut key
    pass

def cut(window): # CTRL+X shortcut key
    sg.popup("<Undo> not implemented", title='TODO', grab_anywhere=True, modal=False)

def copy(window): # CTRL+C shortcut key
    try:
        text = window['_BODY_'].Widget.selection_get()
        window.TKroot.clipboard_clear()
        window.TKroot.clipboard_append(text)
    except:
        print('Nothing selected')

def paste(window): # CTRL+V shortcut key
    ''' paste clipboard contents into current cursor position '''
    try:
        clip = window.TKroot.clipboard_get()
    except:
        return
    else:
        window['_BODY_'].Widget.insert(sg.tk.INSERT, clip)

def delete():
    pass

def find(): # CTRL+F shortcut key
    pass

def replace(): # CTRL+H shortcut key
    pass

def select_all(window): # CTRL+A shortcut key
    ''' select all text in the body '''
    window['_BODY_'].Widget.selection_clear()
    window['_BODY_'].Widget.tag_add("sel","1.0","end")

def fetch_datetime(window, values):
    ''' Append the current date and time into the body '''
    datetime_stamp = datetime.now().strftime("%T %D")
    new_body = values['_BODY_'] + datetime_stamp
    window['_BODY_'].update(value=new_body)    
    settings.update(body=new_body)    


##----FORMAT MENU FUNCTIONS------------------------------##

def change_theme(window, event, values):
    ''' Change the color theme of the application window. This will destroy the active window and 
        recreate it with the same values.'''
    #old_theme = settings['theme']
    #if not old_theme == event:
    #    print(f"Theme.......... {old_theme} => {event}\n")
    settings.update(theme=event, body=values['_BODY_'], out=values['_OUT_'])
    sg.change_look_and_feel(event)
    window.close()

def change_font(window):
    ''' Change default font on body element and save as user settings '''
    # get the default font from user settings
    font_name, font_size = settings.get('font')
    # get available fonts from local environment to use in combo box
    font_list = sorted([f for f in tkfont.families() if f[0]!='@'])
    # check if default font is in font_list, if not return set first font in the list as default
    if not font_name in font_list:
      font_name = font_list[0]
    # available sizes to use for combo box (restricted to practical sizes)
    font_sizes = [8,9,10,11,12,14]
    # setup the font gui window
    font_layout = [
        [sg.Combo(font_list, key='_FONT_', default_value=font_name), 
         sg.Combo(font_sizes, key='_SIZE_', default_value=font_size)],[sg.OK(), sg.Cancel()]]
    font_window = sg.Window('Font', font_layout, keep_on_top=True)
    # listen for font selection events
    font_event, font_values = font_window.read()
    if font_event not in (None,'Exit'):
        font_selection = (font_values['_FONT_'], font_values['_SIZE_'])
        # check to see if the font changed
        if font_selection != settings['font']:
            settings.update(font=font_selection)
            window['_BODY_'].update(font=font_selection)
            window['_OUT_'].update(font=font_selection)
            print(f"Font........... {(font_name, font_size)} => {font_selection}\n")
    font_window.close()

def change_tabsize(window):
    ''' Change the tab size in the body. This is the user interface for the set_tabsize function '''
    tab_layout = [[sg.Slider(range=(1,8), default_value=settings['tabsize'], orientation='horizontal', key='_SIZE_'), sg.OK(size=(5,2))]]
    tab_window = sg.Window('Tab Size', tab_layout, keep_on_top=True)
    tab_event, tab_values = tab_window.read()
    if tab_event not in (None, 'Exit'):
        old_tab_size = settings['tabsize']
        new_tab_size = int(tab_values['_SIZE_'])
        # check to see if tab size changed
        if new_tab_size != old_tab_size:
            settings.update(tabsize=new_tab_size)
            set_tabsize(window, new_tab_size)
            print(f"Tab size....... {old_tab_size} => {new_tab_size}\n")
    tab_window.close()

def set_tabsize(window, size=4): # load upon opening after 'finalize=True' is fixed
    ''' Adjust the tab size in the body; default is 4 '''
    font = tkfont.Font(font=settings.get('font'))
    tab_width = font.measure(' '*size)
    window['_BODY_'].Widget.configure(tabs=(tab_width,)) 
    settings.update(tabsize=size) 

def show_settings():
    ''' Print the saved user settings to the Output element '''
    print(f"Theme.......... {settings['theme']}")
    print(f"Tab size....... {settings['tabsize']}")
    print( "Font.............. {}, {}".format(*settings['font']))
    print(f"Open file...... {settings['filename']}\n")


##----RUN MENU FUNCTIONS---------------------------------##

def run_module(values): # F5 shortcut key
    ''' Run any python code that is in the body '''
    print('.'*50)
    print(f"Running session :: {settings['info']}\n")
    try:
        exec(values['_BODY_'])
    except:
        print('ERROR!......... Invalid Python Code')
    print('.'*50)


##----HELP MENU FUNCTIONS--------------------------------##

def get_help():
    msg = '''
    “Never let your ego get in the way of asking for help 
    when in desperate need. We have all been helped at a 
    point in our lives.” ― Edmond Mbiaka
    '''
    sg.Popup(msg, title='Help', grab_anywhere=True, modal=True)

def about_me():
    msg = '''
    TextCodeEditor is a PySimpleGUI based text and code 
    editor that is being developed as a hobby project.
    It is very much a work in progress. So, if you'd 
    like to contribute to the development, then be my
    guest! 

    Version.......{}
    Date..... {}
    '''.format(VERSION, VERSION_DATE)
    sg.popup(msg, title='About', grab_anywhere=True, modal=True)


##----MAIN EVENT LOOP------------------------------------##

window = main_window(settings)
redir = RedirectText(window)
sys.stdout = redir

while True:
    event, values = window.read(timeout=1)

    # adjust window when application is activated
    if not application_active:
        application_active = True
        set_tabsize(window)
   
    # listen for window events
    if event in (None, 'Exit'):
        close_settings()
        break
    if event in ('Mod Load'):
        global_floder = mod_file(window)
    if event in ('-TREE_LIST-'):
        choose_file(window, global_floder)
    if event in ('New File','n:78'):
        new_file(window)
    if event in ('Open File','o:79'):
        open_file(window)
    if event in ('Save','s:83'):
        save_file(window, values)
    if event in ('Save As',):
        save_file_as(window, values)
    if event in ('Date/Time',):
        fetch_datetime(window, values)
    if event in ('Font',):
        change_font(window)
    if event in ('Tab Size',):
        change_tabsize(window)
    if event in ('Show Settings',):
        show_settings()
    if event in ('Run Module', 'F5:116' ):
        run_module(values)
    if event in settings['themes']: 
        ###############################>>> refactor this bit into a function
        # set application to inactive
        application_active = False
        change_theme(window, event, values)
        # recreate window and redirect object
        sys.stdout = redir.saveout
        window = main_window(settings)
        redir = RedirectText(window)
        sys.stdout = redir
    if event in ('View Help',):
        get_help()
    if event in ('About Me',):
        about_me()
    if event in ('Cut',):
        cut(window)
    if event in ('Copy',):
        copy(window)
    if event in ('Paste',):
        paste(window)
    if event in ('Select All',):
        select_all(window)