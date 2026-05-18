#pragma once

#include "SettingsManager.hpp"
#include <gtkmm.h>

class SettingsWindow : public Gtk::Window {
public:
  SettingsWindow();

private:
  Gtk::Box m_main_box{Gtk::Orientation::VERTICAL, 0};
  Gtk::ListBox m_list_box;

  Gtk::Switch m_paste_switch;
  Gtk::Switch m_multi_emoji_switch;
  Gtk::SpinButton m_timeout_spin;
  Gtk::SpinButton m_column_spin;
  Gtk::DropDown m_skin_tone_dropdown;
  Gtk::Switch m_background_switch;

  Gtk::ListBoxRow *m_timeout_row{nullptr};
  Gtk::ListBoxRow *m_column_row{nullptr};

  enum class ExtensionState {
    NOT_INSTALLED = 0,
    ENABLED = 1,
    DISABLED = 2,
    ERROR = 3,
    UNKNOWN = -1
  };
  struct ExtensionStatus {
    ExtensionState state = ExtensionState::NOT_INSTALLED;
    bool is_enabled = false;
    Glib::ustring error_details = "";
  };

  Gtk::Label *m_status_badge = nullptr;
  Gtk::Button *m_ext_btn = nullptr;
  Gtk::ListBoxRow *m_paste_row = nullptr;

  void refresh_extension_status();
  ExtensionStatus get_emojify_status();
  void setup_list();
  void setup_bindings();
  void setup_extension_section();
  void setup_behavior_section();
  void setup_appearance_section();
  void setup_system_section();
  void on_show();
  Gtk::ListBox *make_section(const Glib::ustring &title);
  Gtk::ListBoxRow *make_row(const Glib::ustring &title,
                            const Glib::ustring &subtitle, Gtk::Widget &widget);
};