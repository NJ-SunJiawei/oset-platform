#include "menu.hpp"
#include "config.hpp"
#include "terminal.hpp"
#include <iostream>
#include <string>

const Glib::ustring menu_xml = R"RAW(<interface>
  <menu id='right-click-line-menu'>
    <section>
      <item>
        <attribute name='label' translatable='yes'>_Undo</attribute>
        <attribute name='action'>app.edit_undo</attribute>
      </item>
      <item>
        <attribute name='label' translatable='yes'>_Redo</attribute>
        <attribute name='action'>app.edit_redo</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name='label' translatable='yes'>_Paste</attribute>
        <attribute name='action'>app.edit_paste</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name='label' translatable='yes'>_Toggle _Comments</attribute>
        <attribute name='action'>app.source_comments_toggle</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name='label' translatable='yes'>_Go _to _Declaration</attribute>
        <attribute name='action'>app.source_goto_declaration</attribute>
      </item>
      <item>
        <attribute name='label' translatable='yes'>_Go _to _Type _Declaration</attribute>
        <attribute name='action'>app.source_goto_type_declaration</attribute>
      </item>
      <item>
        <attribute name='label' translatable='yes'>_Go _to _Implementation</attribute>
        <attribute name='action'>app.source_goto_implementation</attribute>
      </item>
      <item>
        <attribute name='label' translatable='yes'>_Go _to _Usage</attribute>
        <attribute name='action'>app.source_goto_usage</attribute>
      </item>
      <item>
        <attribute name='label' translatable='yes'>_Go _to _Method</attribute>
        <attribute name='action'>app.source_goto_method</attribute>
      </item>
      <item>
        <attribute name='label' translatable='yes'>_Rename</attribute>
        <attribute name='action'>app.source_rename</attribute>
      </item>
    </section>
  </menu>
  <menu id='right-click-selected-menu'>
    <section>
      <item>
        <attribute name='label' translatable='yes'>_Undo</attribute>
        <attribute name='action'>app.edit_undo</attribute>
      </item>
      <item>
        <attribute name='label' translatable='yes'>_Redo</attribute>
        <attribute name='action'>app.edit_redo</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name='label' translatable='yes'>_Cut</attribute>
        <attribute name='action'>app.edit_cut</attribute>
      </item>
      <item>
        <attribute name='label' translatable='yes'>_Copy</attribute>
        <attribute name='action'>app.edit_copy</attribute>
      </item>
      <item>
        <attribute name='label' translatable='yes'>_Paste</attribute>
        <attribute name='action'>app.edit_paste</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name='label' translatable='yes'>_Toggle _Comments</attribute>
        <attribute name='action'>app.source_comments_toggle</attribute>
      </item>
    </section>
  </menu>

  <menu id='juci-menu'>
    <section>
      <item>
        <attribute name='label' translatable='yes'>_About</attribute>
        <attribute name='action'>app.about</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name='label' translatable='yes'>_Preferences</attribute>
        <attribute name='action'>app.preferences</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name='label' translatable='yes'>_Snippets</attribute>
        <attribute name='action'>app.snippets</attribute>
      </item>
      <item>
        <attribute name='label' translatable='yes'>_Commands</attribute>
        <attribute name='action'>app.commands</attribute>
      </item>
    </section>
    <section>
      <item>
        <attribute name='label' translatable='yes'>_Quit</attribute>
        <attribute name='action'>app.quit</attribute>
      </item>
    </section>
  </menu>

  <menu id='window-menu'>
    <submenu>
	  <attribute name='label' translatable='yes'>_Oset</attribute>
	  <section>
	    <item>
		  <attribute name='label' translatable='yes'>_Core _Setting</attribute>
		  <attribute name='action'>app.core_init_setting</attribute>
	    </item>
	    <item>
		  <attribute name='label' translatable='yes'>_Mod _Setting</attribute>
		  <attribute name='action'>app.mod_init_setting</attribute>
	    </item>
	    <item>
		  <attribute name='label' translatable='yes'>_Esl _Setting</attribute>
		  <attribute name='action'>app.esl_init_setting</attribute>
	    </item>
	  </section>
	  <section>
	    <item>
		  <attribute name='label' translatable='yes'>_Oset _Run</attribute>
		  <attribute name='action'>app.oset_run</attribute>
	    </item>
	    <item>
		  <attribute name='label' translatable='yes'>_Oset _Stop</attribute>
		  <attribute name='action'>app.oset_stop</attribute>
	    </item>
	  <item>
		<attribute name='label' translatable='yes'>_Oset _Command</attribute>
		<attribute name='action'>app.oset_command</attribute>
	  </item>
	  <item>
		<attribute name='label' translatable='yes'>_Oset _Xcli</attribute>
		<attribute name='action'>app.oset_xcli</attribute>
	  </item>
	  <item>
		<attribute name='label' translatable='yes'>_Oset _Xdebug</attribute>
		<attribute name='action'>app.oset_xdebug</attribute>
	  </item>
	  </section>
    </submenu>


    <submenu>
      <attribute name='label' translatable='yes'>_File</attribute>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_New _File</attribute>
          <attribute name='action'>app.file_new_file</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_New _Folder</attribute>
          <attribute name='action'>app.file_new_folder</attribute>
        </item>
        <submenu>
          <attribute name='label' translatable='yes'>_New _Project</attribute>
          <item>
            <attribute name='label' translatable='no'>Oset</attribute>
            <attribute name='action'>app.file_new_project_oset</attribute>
          </item>
          <item>
            <attribute name='label' translatable='no'>C</attribute>
            <attribute name='action'>app.file_new_project_c</attribute>
          </item>
          <item>
            <attribute name='label' translatable='no'>C++</attribute>
            <attribute name='action'>app.file_new_project_cpp</attribute>
          </item>
          <item>
            <attribute name='label' translatable='no'>Rust</attribute>
            <attribute name='action'>app.file_new_project_rust</attribute>
          </item>
        </submenu>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Open _File</attribute>
          <attribute name='action'>app.file_open_file</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Open _Folder</attribute>
          <attribute name='action'>app.file_open_folder</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Find _File</attribute>
          <attribute name='action'>app.file_find_file</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Switch _File _Type</attribute>
          <attribute name='action'>app.file_switch_file_type</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Reload _File</attribute>
          <attribute name='action'>app.file_reload_file</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Save</attribute>
          <attribute name='action'>app.file_save</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Save _As</attribute>
          <attribute name='action'>app.file_save_as</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Close _File</attribute>
          <attribute name='action'>app.file_close_file</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Close _Folder</attribute>
          <attribute name='action'>app.file_close_folder</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Close _Project</attribute>
          <attribute name='action'>app.file_close_project</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Close _Other _Files</attribute>
          <attribute name='action'>app.file_close_other_files</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Print</attribute>
          <attribute name='action'>app.file_print</attribute>
        </item>
      </section>
    </submenu>

    <submenu>
      <attribute name='label' translatable='yes'>_Edit</attribute>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Undo</attribute>
          <attribute name='action'>app.edit_undo</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Redo</attribute>
          <attribute name='action'>app.edit_redo</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Cut</attribute>
          <attribute name='action'>app.edit_cut</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Cut _Lines</attribute>
          <attribute name='action'>app.edit_cut_lines</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Copy</attribute>
          <attribute name='action'>app.edit_copy</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Copy _Lines</attribute>
          <attribute name='action'>app.edit_copy_lines</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Paste</attribute>
          <attribute name='action'>app.edit_paste</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Extend _Selection</attribute>
          <attribute name='action'>app.edit_extend_selection</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Shrink _Selection</attribute>
          <attribute name='action'>app.edit_shrink_selection</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Show/_Hide</attribute>
          <attribute name='action'>app.edit_show_or_hide</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Find</attribute>
          <attribute name='action'>app.edit_find</attribute>
        </item>
      </section>
    </submenu>

    <submenu>
      <attribute name='label' translatable='yes'>_Source</attribute>
      <section>
        <submenu>
          <attribute name='label' translatable='yes'>_Spell _Check</attribute>
          <item>
            <attribute name='label' translatable='yes'>_Spell _Check _Buffer</attribute>
            <attribute name='action'>app.source_spellcheck</attribute>
          </item>
          <item>
            <attribute name='label' translatable='yes'>_Clear _Spelling _Errors</attribute>
            <attribute name='action'>app.source_spellcheck_clear</attribute>
          </item>
          <item>
            <attribute name='label' translatable='yes'>_Go _to _Next _Spelling _Error</attribute>
            <attribute name='action'>app.source_spellcheck_next_error</attribute>
          </item>
        </submenu>
      </section>
      <section>
        <submenu>
          <attribute name='label' translatable='yes'>_Git</attribute>
          <item>
            <attribute name='label' translatable='yes'>_Go _to _Next _Diff</attribute>
            <attribute name='action'>app.source_git_next_diff</attribute>
          </item>
          <item>
            <attribute name='label' translatable='yes'>_Show _Diff</attribute>
            <attribute name='action'>app.source_git_show_diff</attribute>
          </item>
        </submenu>
      </section>
      <section>
        <submenu>
          <attribute name='label' translatable='yes'>_Indentation</attribute>
          <item>
            <attribute name='label' translatable='yes'>_Set _Current _Buffer _Tab</attribute>
            <attribute name='action'>app.source_indentation_set_buffer_tab</attribute>
          </item>
          <item>
            <attribute name='label' translatable='yes'>_Auto-Indent _Current _Buffer</attribute>
            <attribute name='action'>app.source_indentation_auto_indent_buffer</attribute>
          </item>
        </submenu>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Go _to _Line</attribute>
          <attribute name='action'>app.source_goto_line</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Center _Cursor</attribute>
          <attribute name='action'>app.source_center_cursor</attribute>
        </item>
        <submenu>
          <attribute name='label' translatable='yes'>_Cursor _History</attribute>
          <item>
            <attribute name='label' translatable='yes'>_Back</attribute>
            <attribute name='action'>app.source_cursor_history_back</attribute>
          </item>
          <item>
            <attribute name='label' translatable='yes'>_Forward</attribute>
            <attribute name='action'>app.source_cursor_history_forward</attribute>
          </item>
        </submenu>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Find _Symbol</attribute>
          <attribute name='action'>app.source_find_symbol</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Find _Pattern</attribute>
          <attribute name='action'>app.source_find_pattern</attribute>
        </item>
      </section>
      <section>
        <submenu>
          <attribute name='label' translatable='yes'>_Comments</attribute>
          <item>
            <attribute name='label' translatable='yes'>_Toggle _Comments</attribute>
            <attribute name='action'>app.source_comments_toggle</attribute>
          </item>
          <item>
            <attribute name='label' translatable='yes'>_Add _Documentation</attribute>
            <attribute name='action'>app.source_comments_add_documentation</attribute>
          </item>
        </submenu>
        <item>
          <attribute name='label' translatable='yes'>_Find _Documentation</attribute>
          <attribute name='action'>app.source_find_documentation</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Go to Declaration</attribute>
          <attribute name='action'>app.source_goto_declaration</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Go to _Type _Declaration</attribute>
          <attribute name='action'>app.source_goto_type_declaration</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Go to Implementation</attribute>
          <attribute name='action'>app.source_goto_implementation</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Go to Usage</attribute>
          <attribute name='action'>app.source_goto_usage</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Go to Method</attribute>
          <attribute name='action'>app.source_goto_method</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Rename</attribute>
          <attribute name='action'>app.source_rename</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Implement _Method</attribute>
          <attribute name='action'>app.source_implement_method</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Go to Next Diagnostic</attribute>
          <attribute name='action'>app.source_goto_next_diagnostic</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Apply Fix-Its</attribute>
          <attribute name='action'>app.source_apply_fix_its</attribute>
        </item>
      </section>
    </submenu>

    <submenu>
      <attribute name='label' translatable='yes'>_Project</attribute>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Set _Run _Arguments</attribute>
          <attribute name='action'>app.project_set_run_arguments</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Compile</attribute>
          <attribute name='action'>app.project_compile</attribute>
        </item>
         <item>
          <attribute name='label' translatable='yes'>_Compile _and _Run</attribute>
          <attribute name='action'>app.project_compile_and_run</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Oset _mod _install</attribute>
          <attribute name='action'>app.project_oset_mod_install</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Oset _mod _uninstall</attribute>
          <attribute name='action'>app.project_oset_mod_uninstall</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Recreate _Build</attribute>
          <attribute name='action'>app.project_recreate_build</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Run _Command</attribute>
          <attribute name='action'>app.project_run_command</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Kill _Last _Process</attribute>
          <attribute name='action'>app.project_kill_last_running</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Force _Kill _Last _Process</attribute>
          <attribute name='action'>app.project_force_kill_last_running</attribute>
        </item>
      </section>
    </submenu>

    <submenu>
      <attribute name='label' translatable='yes'>_Debug</attribute>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Set _Run _Arguments</attribute>
          <attribute name='action'>app.debug_set_run_arguments</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Start/_Continue</attribute>
          <attribute name='action'>app.debug_start_continue</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Stop</attribute>
          <attribute name='action'>app.debug_stop</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Kill</attribute>
          <attribute name='action'>app.debug_kill</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Step _Over</attribute>
          <attribute name='action'>app.debug_step_over</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Step _Into</attribute>
          <attribute name='action'>app.debug_step_into</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Step _Out</attribute>
          <attribute name='action'>app.debug_step_out</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Backtrace</attribute>
          <attribute name='action'>app.debug_backtrace</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Show _Variables</attribute>
          <attribute name='action'>app.debug_show_variables</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Run Command</attribute>
          <attribute name='action'>app.debug_run_command</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Toggle _Breakpoint</attribute>
          <attribute name='action'>app.debug_toggle_breakpoint</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Show _Breakpoints</attribute>
          <attribute name='action'>app.debug_show_breakpoints</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Go _to _Stop</attribute>
          <attribute name='action'>app.debug_goto_stop</attribute>
        </item>
      </section>
    </submenu>

    <submenu>
      <attribute name='label' translatable='yes'>_Window</attribute>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Next _Tab</attribute>
          <attribute name='action'>app.window_next_tab</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Previous _Tab</attribute>
          <attribute name='action'>app.window_previous_tab</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Go _to _Tab</attribute>
          <attribute name='action'>app.window_goto_tab</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Toggle _Split</attribute>
          <attribute name='action'>app.window_toggle_split</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Split _Source _Buffer</attribute>
          <attribute name='action'>app.window_split_source_buffer</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Toggle _Full _Screen</attribute>
          <attribute name='action'>app.window_toggle_full_screen</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Toggle _Directories</attribute>
          <attribute name='action'>app.window_toggle_directories</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Toggle _Terminal</attribute>
          <attribute name='action'>app.window_toggle_terminal</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Toggle _Status</attribute>
          <attribute name='action'>app.window_toggle_status</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Toggle _Menu</attribute>
          <attribute name='action'>app.window_toggle_menu</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Toggle _Tabs</attribute>
          <attribute name='action'>app.window_toggle_tabs</attribute>
        </item>
        <item>
          <attribute name='label' translatable='yes'>_Toggle _Zen _Mode</attribute>
          <attribute name='action'>app.window_toggle_zen_mode</attribute>
        </item>
      </section>
      <section>
        <item>
          <attribute name='label' translatable='yes'>_Clear _Terminal</attribute>
          <attribute name='action'>app.window_clear_terminal</attribute>
        </item>
      </section>
    </submenu>
  </menu>
</interface>
)RAW";

void Menu::add_action(const std::string &name, const std::function<void()> &action) {
  actions[name] = Glib::RefPtr<Gtk::Application>::cast_dynamic(Gtk::Application::get_default())->add_action(name, action);
}

void Menu::set_keys() {
  auto application = Glib::RefPtr<Gtk::Application>::cast_dynamic(Gtk::Application::get_default());

  accelerators_with_multiple_actions.clear();

  for(auto &action_and_key : Config::get().menu.keys) {
    auto it = actions.find(action_and_key.first);
    if(it != actions.end()) {
      if(!action_and_key.second.empty()) {
        application->set_accel_for_action("app." + action_and_key.first, action_and_key.second);

        guint key = 0;
        GdkModifierType modifier = static_cast<GdkModifierType>(0);
        gtk_accelerator_parse(action_and_key.second.c_str(), &key, &modifier);
        if(key == 0 && modifier == 0)
          Terminal::get().async_print("\e[31mError\e[m: could not parse key string \"" + action_and_key.second + "\" for action " + action_and_key.first + "\n", true);
        else
          accelerators_with_multiple_actions[std::make_pair(key, modifier)].emplace_back(it->second);
      }
      else
        application->unset_accels_for_action("app." + action_and_key.first);
    }
  }

  for(auto it = accelerators_with_multiple_actions.begin(); it != accelerators_with_multiple_actions.end();) {
    if(it->second.size() < 2)
      it = accelerators_with_multiple_actions.erase(it);
    else
      ++it;
  }
}

void Menu::build() {
  try {
    builder = Gtk::Builder::create_from_string(menu_xml);
    auto object = builder->get_object("juci-menu");
    juci_menu = Glib::RefPtr<Gio::Menu>::cast_dynamic(object);
    object = builder->get_object("window-menu");
    window_menu = Glib::RefPtr<Gio::Menu>::cast_dynamic(object);
    object = builder->get_object("right-click-line-menu");
    auto ptr = Glib::RefPtr<Gio::Menu>::cast_dynamic(object);
    right_click_line_menu = std::make_unique<Gtk::Menu>(ptr);
    object = builder->get_object("right-click-selected-menu");
    ptr = Glib::RefPtr<Gio::Menu>::cast_dynamic(object);
    right_click_selected_menu = std::make_unique<Gtk::Menu>(ptr);
  }
  catch(const Glib::Error &e) {
    std::cerr << "building menu failed: " << e.what();
  }
}
