#include "windows/SettingsWindow.hpp"
#include "config.h"

static const std::array<Glib::ustring, 6> SKIN_TONE_LABELS = {
    "👍️", "👍🏻", "👍🏼", "👍🏽", "👍🏾", "👍🏿"};

static constexpr const char *EXTENSION_URL =
    "https://extensions.gnome.org/extension/9987/emojify-bridge/";
static constexpr const char *EXTENSION_UUID = "emojify-extension@riothedev.xyz";

SettingsWindow::SettingsWindow() {
  set_title("Preferences");
  set_default_size(600, 0);
  set_resizable(false);

  m_main_box.set_margin(0);
  set_child(m_main_box);

  setup_list();
  setup_bindings();
  property_is_active().signal_changed().connect(
      [this]() { refresh_extension_status(); });
}
void SettingsWindow::on_show() {
  g_print("ran");
  Gtk::Window::on_show();
  refresh_extension_status();
}

SettingsWindow::ExtensionStatus SettingsWindow::get_emojify_status() {
  ExtensionStatus status;

  try {
    auto bus = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SESSION);
    auto parameters = Glib::VariantContainerBase::create_tuple(
        Glib::Variant<Glib::ustring>::create(EXTENSION_UUID));

    auto result =
        bus->call_sync("/org/gnome/Shell", "org.gnome.Shell.Extensions",
                       "GetExtensionInfo", parameters, "org.gnome.Shell");

    Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>> info_variant;
    result.get_child(info_variant, 0);
    auto metadata = info_variant.get();

    if (auto it = metadata.find("state"); it != metadata.end()) {
      double state_val =
          Glib::VariantBase::cast_dynamic<Glib::Variant<double>>(it->second)
              .get();

      if (state_val == 1.0) {
        status.state = ExtensionState::ENABLED;
        status.is_enabled = true;
      } else if (state_val == 2.0) {
        status.state = ExtensionState::DISABLED;
      } else if (state_val == 3.0) {
        status.state = ExtensionState::ERROR;
      } else {
        status.state = ExtensionState::UNKNOWN;
      }
    }
  } catch (const Glib::Error &ex) {
    status.state = ExtensionState::NOT_INSTALLED;
    status.error_details = ex.what();
  }

  return status;
}

Gtk::ListBox *SettingsWindow::make_section(const Glib::ustring &title) {
  auto *header = Gtk::make_managed<Gtk::Label>(title);
  header->set_halign(Gtk::Align::START);
  header->set_margin_start(12);
  header->set_margin_top(16);
  header->set_margin_bottom(6);
  header->get_style_context()->add_class("heading");

  auto *list = Gtk::make_managed<Gtk::ListBox>();
  list->set_selection_mode(Gtk::SelectionMode::NONE);
  list->set_margin_start(12);
  list->set_margin_end(12);
  list->set_margin_bottom(4);
  list->get_style_context()->add_class("boxed-list");

  m_main_box.append(*header);
  m_main_box.append(*list);
  return list;
}

Gtk::ListBoxRow *SettingsWindow::make_row(const Glib::ustring &title,
                                          const Glib::ustring &subtitle,
                                          Gtk::Widget &widget) {
  auto *row = Gtk::make_managed<Gtk::ListBoxRow>();
  auto *hbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
  hbox->set_margin(12);

  auto *label_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
  label_box->set_hexpand(true);
  label_box->set_valign(Gtk::Align::CENTER);

  auto *title_label = Gtk::make_managed<Gtk::Label>(title);
  title_label->set_halign(Gtk::Align::START);
  label_box->append(*title_label);

  if (!subtitle.empty()) {
    auto *sub_label = Gtk::make_managed<Gtk::Label>(subtitle);
    sub_label->set_halign(Gtk::Align::START);
    sub_label->set_wrap(true);
    sub_label->set_max_width_chars(42);
    sub_label->get_style_context()->add_class("dim-label");

    Pango::AttrList attrs;
    auto attr = Pango::Attribute::create_attr_scale(Pango::SCALE_SMALL);
    attrs.insert(attr);
    sub_label->set_attributes(attrs);

    label_box->append(*sub_label);
  }

  hbox->append(*label_box);
  hbox->append(widget);
  row->set_child(*hbox);
  return row;
}
void SettingsWindow::refresh_extension_status() {
  auto status = get_emojify_status();

  Glib::ustring badge_text;
  Glib::ustring badge_css;

  switch (status.state) {
  case ExtensionState::ENABLED:
    badge_text = "● Active";
    badge_css = "success";
    break;
  case ExtensionState::DISABLED:
    badge_text = "● Disabled";
    badge_css = "warning";
    break;
  case ExtensionState::ERROR:
    badge_text = "● Error";
    badge_css = "error";
    break;
  case ExtensionState::NOT_INSTALLED:
  default:
    badge_text = "● Not installed";
    badge_css = "error";
    break;
  }

  m_status_badge->set_text(badge_text);
  for (auto cls : {"success", "warning", "error"})
    m_status_badge->get_style_context()->remove_class(cls);
  m_status_badge->get_style_context()->add_class(badge_css);

  if (!status.error_details.empty())
    m_status_badge->set_tooltip_text(status.error_details);

  bool installed = status.state == ExtensionState::ENABLED;
  m_ext_btn->set_sensitive(!installed);

  m_paste_row->set_tooltip_text(
      status.is_enabled ? "Extension is active — paste on select is available"
                        : "Install and enable the Emojify GNOME Shell "
                          "extension to use this feature");

  auto *btn_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
  auto *icon = Gtk::make_managed<Gtk::Image>();
  auto *label =
      Gtk::make_managed<Gtk::Label>(installed ? "Installed" : "Get Extension");
  icon->set_from_icon_name(installed ? "emblem-ok-symbolic"
                                     : "adw-external-link-symbolic");
  btn_box->append(*icon);
  btn_box->append(*label);
  m_ext_btn->set_child(*btn_box);
  m_ext_btn->set_sensitive(!installed);
}
void SettingsWindow::setup_extension_section() {
  auto *list = make_section("Extension");

  auto *paste_box =
      Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
  paste_box->set_valign(Gtk::Align::CENTER);

  m_status_badge = Gtk::make_managed<Gtk::Label>();
  m_status_badge->get_style_context()->add_class("caption");
  m_status_badge->set_valign(Gtk::Align::CENTER);
  m_paste_switch.set_valign(Gtk::Align::CENTER);

  paste_box->append(*m_status_badge);
  paste_box->append(m_paste_switch);

  m_paste_row =
      make_row("Paste automatically",
               "Paste the selected emoji directly into the focused window "
               "(requires the GNOME Shell extension)",
               *paste_box);
  list->append(*m_paste_row);

  m_ext_btn = Gtk::make_managed<Gtk::Button>();
  m_ext_btn->set_valign(Gtk::Align::CENTER);
  m_ext_btn->signal_clicked().connect([]() {
    Gio::AppInfo::launch_default_for_uri(EXTENSION_URL,
                                         Glib::RefPtr<Gio::AppLaunchContext>());
  });

  list->append(
      *make_row("GNOME Shell Extension",
                "Required for automatic pasting — opens extensions.gnome.org",
                *m_ext_btn));

  refresh_extension_status();
}
void SettingsWindow::setup_behavior_section() {
  auto *list = make_section("Behavior");

  m_multi_emoji_switch.set_valign(Gtk::Align::CENTER);
  list->append(*make_row("Multi-emoji mode",
                         "Select multiple emojis consecutively before pasting",
                         m_multi_emoji_switch));

  m_timeout_spin.set_range(50, 5000);
  m_timeout_spin.set_increments(50, 100);
  m_timeout_spin.set_numeric(true);
  m_timeout_spin.set_valign(Gtk::Align::CENTER);
  m_timeout_spin.set_size_request(90, -1);
  m_timeout_row =
      make_row("Paste delay (ms)",
               "Time to wait after the last emoji selection before pasting",
               m_timeout_spin);
  list->append(*m_timeout_row);
}

void SettingsWindow::setup_appearance_section() {
  auto *list = make_section("Appearance");

  auto string_list = Gtk::StringList::create(std::vector<Glib::ustring>(
      SKIN_TONE_LABELS.begin(), SKIN_TONE_LABELS.end()));
  m_skin_tone_dropdown.set_model(string_list);
  m_skin_tone_dropdown.set_valign(Gtk::Align::CENTER);
  list->append(*make_row("Skin tone",
                         "Default skin tone used for emojis that support it",
                         m_skin_tone_dropdown));

  m_column_spin.set_range(3, 10);
  m_column_spin.set_increments(1, 2);
  m_column_spin.set_numeric(true);
  m_column_spin.set_valign(Gtk::Align::CENTER);
  m_column_spin.set_size_request(90, -1);
  m_column_row =
      make_row("Columns", "Number of columns in the emoji grid", m_column_spin);
  list->append(*m_column_row);
}

void SettingsWindow::setup_system_section() {
  auto *list = make_section("System");

  m_background_switch.set_valign(Gtk::Align::CENTER);
  list->append(
      *make_row("Run in background",
                "Keep the app running after closing for faster startup",
                m_background_switch));

  Glib::ustring command;
  if (getenv("FLATPAK_ID") != nullptr)
    command = "flatpak run xyz.riothedev.emojify";
  else if (const char *appimage = getenv("APPIMAGE"); appimage != nullptr)
    command = appimage;
  else
    command = "/usr/bin/emojify";

  auto *entry_box =
      Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);

  auto *entry = Gtk::make_managed<Gtk::Entry>();
  entry->set_text(command);
  entry->set_editable(false);
  entry->set_hexpand(false);
  entry->set_valign(Gtk::Align::CENTER);
  entry->set_width_chars(26);

  auto *copy_btn = Gtk::make_managed<Gtk::Button>();
  copy_btn->set_icon_name("edit-copy-symbolic");
  copy_btn->set_tooltip_text("Copy to clipboard");
  copy_btn->set_valign(Gtk::Align::CENTER);
  copy_btn->signal_clicked().connect(
      [entry]() { entry->get_clipboard()->set_text(entry->get_text()); });

  entry_box->append(*entry);
  entry_box->append(*copy_btn);

  list->append(*make_row("Keyboard shortcut",
                         "Add this command to your system's keyboard settings "
                         "to open Emojify with a shortcut",
                         *entry_box));
}

void SettingsWindow::setup_list() {
  setup_extension_section();
  setup_behavior_section();
  setup_appearance_section();
  setup_system_section();

  auto *version_label = Gtk::make_managed<Gtk::Label>();
  version_label->set_text(Glib::ustring::compose(
      "Emojify v%1 • Built with Meson %2", APP_VERSION, MESON_BUILD_VERSION));
  version_label->set_halign(Gtk::Align::CENTER);
  version_label->set_margin_top(8);
  version_label->set_margin_bottom(16);
  version_label->get_style_context()->add_class("dim-label");
  version_label->get_style_context()->add_class("caption");
  m_main_box.append(*version_label);
}

void SettingsWindow::setup_bindings() {
  auto settings = SettingsManager::get_instance().get_settings();

  settings->bind("paste-on-select", &m_paste_switch, "active");
  settings->bind("multi-emoji", &m_multi_emoji_switch, "active");
  settings->bind("timeout-ms", &m_timeout_spin, "value");
  settings->bind("columns", &m_column_spin, "value");
  settings->bind("run-in-background", &m_background_switch, "active");

  auto sync_timeout = [this]() {
    m_timeout_row->set_sensitive(m_multi_emoji_switch.get_active());
  };
  m_multi_emoji_switch.property_active().signal_changed().connect(sync_timeout);
  sync_timeout();

  m_skin_tone_dropdown.set_selected(settings->get_enum("skin-tone"));
  m_skin_tone_dropdown.property_selected().signal_changed().connect(
      [this, settings]() {
        settings->set_enum(
            "skin-tone", static_cast<int>(m_skin_tone_dropdown.get_selected()));
      });
}