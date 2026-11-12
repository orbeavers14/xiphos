// SPDX-License-Identifier: GPL-2.0-or-later
#include "sync_ui.hpp"
#include <memory>

namespace xiphos::linux_preview {
namespace {
std::string copy(const char *s) { return s ? s : ""; }
const char *text(GtkWidget *widget) { return gtk_editable_get_text(GTK_EDITABLE(widget)); }
bool checked(GtkWidget *widget) { return gtk_check_button_get_active(GTK_CHECK_BUTTON(widget)); }
}

SyncUI::SyncUI(GtkApplication *application, xc_worker &worker)
    : worker_(worker), available_(xc_sync_network_available() != 0), application_(application)
{
  window_ = GTK_WINDOW(gtk_application_window_new(application));
  g_object_ref(window_);
  gtk_window_set_title(window_, "BibleSync passage sharing");
  gtk_window_set_default_size(window_, 480, 560);
  gtk_window_set_hide_on_close(window_, TRUE);
  g_signal_connect(window_, "hide", G_CALLBACK(+[](GtkWidget *widget, gpointer) {
    gtk_window_set_application(GTK_WINDOW(widget), nullptr);
  }), nullptr);
  auto *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_window_set_child(window_, box);
  auto append = [&](GtkWidget *widget) {
    gtk_box_append(GTK_BOX(box), widget);
    return widget;
  };
  auto field = [&](const char *label) {
    auto *widget = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(widget), label);
    gtk_accessible_update_property(GTK_ACCESSIBLE(widget), GTK_ACCESSIBLE_PROPERTY_LABEL, label, -1);
    return append(widget);
  };
  const char *modes[] = {"Personal", "Speaker", "Audience", nullptr};
  mode_ = append(gtk_drop_down_new_from_strings(modes));
  const char *listeners[] = {"Receive from nobody", "Receive from allowed speaker IDs", "Receive from everyone in the session", nullptr};
  listen_ = append(gtk_drop_down_new_from_strings(listeners));
  user_ = field("Display name");
  phrase_ = field("Session phrase");
  gtk_entry_set_visibility(GTK_ENTRY(phrase_), FALSE);
  group_ = field("Group (optional digit 1–9)");
  allowed_ = field("Allowed speaker IDs, separated by commas");
  consent_ = append(gtk_check_button_new_with_label("Allow network sharing. Name and phrase are sent in plaintext."));
  privacy_ = append(gtk_check_button_new_with_label("Keep personal mode private"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(privacy_), TRUE);
  direct_ = append(gtk_check_button_new_with_label("Open received single verses automatically"));
  group_windows_ = append(gtk_check_button_new_with_label("Route groups to their corresponding reading windows"));
  connect_ = append(gtk_button_new_with_label("Connect"));
  disconnect_ = append(gtk_button_new_with_label("Disconnect"));
  send_ = append(gtk_button_new_with_label("Share current passage"));
  review_list_ = append(gtk_drop_down_new(nullptr, nullptr));
  review_button_ = append(gtk_button_new_with_label("Preview received references"));
  status_ = append(gtk_label_new(""));
  gtk_label_set_wrap(GTK_LABEL(status_), TRUE);
  set_status(available_ ? "Disconnected. BibleSync shares passages." : "This build does not include BibleSync networking.");
  g_signal_connect(connect_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
    static_cast<SyncUI *>(data)->configure(true);
  }), this);
  g_signal_connect(disconnect_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
    static_cast<SyncUI *>(data)->configure(false);
  }), this);
  g_signal_connect(send_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
    static_cast<SyncUI *>(data)->send();
  }), this);
  g_signal_connect(review_button_, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
    static_cast<SyncUI *>(data)->review();
  }), this);
  tick();
}
SyncUI::~SyncUI()
{
  xc_request_release(request_);
  if (configured_) {
    xc_request *disconnect = nullptr;
    const xc_sync_settings disabled{};
    xc_worker_configure_sync(&worker_, &disabled, &disconnect);
    xc_request_release(disconnect);
  }
  gtk_window_destroy(window_);
  g_object_unref(window_);
}
void SyncUI::present()
{
  gtk_window_set_application(window_, application_);
  gtk_window_present(window_);
}
void SyncUI::set_status(const char *message) { gtk_label_set_text(GTK_LABEL(status_), message); }
bool SyncUI::submit(xc_status status, xc_request *request, Operation operation)
{
  if (status != XC_OK || !request) {
    xc_request_release(request);
    set_status("BibleSync request was rejected.");
    return false;
  }
  request_ = request;
  operation_ = operation;
  return true;
}
void SyncUI::configure(bool enabled)
{
  if (request_ || !available_)
    return;
  if (enabled && !checked(consent_)) {
    set_status("Acknowledge network sharing to connect.");
    return;
  }
  xc_sync_settings settings{};
  xc_sync_connection connection{};
  std::unique_ptr<gchar *, decltype(&g_strfreev)> parts(g_strsplit(text(allowed_), ",", -1), g_strfreev);
  std::vector<const char *> speakers;
  if (enabled) {
    settings.mode = gtk_drop_down_get_selected(GTK_DROP_DOWN(mode_)) + 1;
    settings.listen = gtk_drop_down_get_selected(GTK_DROP_DOWN(listen_));
    settings.private_personal = checked(privacy_);
    settings.direct_navigation = checked(direct_);
    settings.group_tabs = checked(group_windows_);
    settings.group = text(group_);
    settings.domain = "";
    if (settings.listen == 1)
      for (auto **speaker = parts.get(); *speaker; ++speaker)
        if (*g_strstrip(*speaker))
          speakers.push_back(*speaker);
    settings.allowed_speakers = speakers.data();
    settings.speaker_count = speakers.size();
    connection = {"Xiphos Native Preview", "1", text(user_), text(phrase_), 1};
  }
  xc_request *request = nullptr;
  const auto status = enabled ? xc_worker_configure_sync_network(&worker_, &settings, &connection, &request)
                              : xc_worker_configure_sync(&worker_, &settings, &request);
  if (!submit(status, request, enabled ? Operation::connect : Operation::disconnect))
    return;
  configured_ |= enabled;
  mode_applied_ = settings.mode;
  group_applied_ = copy(settings.group);
  active_ = false;
  incoming_.clear();
  review_.clear();
  update_review();
  gtk_editable_set_text(GTK_EDITABLE(phrase_), "");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(consent_), FALSE);
}
void SyncUI::send()
{
  if (request_ || !active_ || mode_applied_ == 3 || !current_passage)
    return;
  const auto [module, reference] = current_passage();
  if (module.empty() || reference.empty()) {
    set_status("Open a Bible passage to share.");
    return;
  }
  const xc_sync_navigation navigation{"local", module.c_str(), reference.c_str(),
    group_applied_.empty() ? "1" : group_applied_.c_str(), ""};
  xc_request *request = nullptr;
  auto status = xc_worker_transmit_sync_network(&worker_, &navigation, 0, &request);
  submit(status, request, Operation::send);
}
void SyncUI::update_review()
{
  auto *list = gtk_string_list_new(nullptr);
  for (const auto &item : review_)
    gtk_string_list_append(list, (item.speaker + " · " + item.module + " · " + item.reference).c_str());
  gtk_drop_down_set_model(GTK_DROP_DOWN(review_list_), G_LIST_MODEL(list));
  g_object_unref(list);
}
void SyncUI::review()
{
  const auto index = gtk_drop_down_get_selected(GTK_DROP_DOWN(review_list_));
  if (index >= review_.size() || !receive)
    return;
  const auto &item = review_[index];
  set_status(receive(item.module, item.reference, -1, false) ? "Opened received references." :
    "The passage is unavailable. Check the receiving reader and installed modules.");
}
void SyncUI::tick()
{
  const auto state = request_ ? xc_request_poll(request_) : XC_PENDING;
  if (request_ && state != XC_PENDING) {
    std::unique_ptr<xc_request, decltype(&xc_request_release)> finished(request_, xc_request_release);
    request_ = nullptr;
    if (state != XC_OK) {
      if (operation_ == Operation::connect || operation_ == Operation::disconnect || operation_ == Operation::poll)
        active_ = false;
      set_status("BibleSync operation failed.");
    } else if (operation_ == Operation::connect || operation_ == Operation::disconnect) {
      active_ = operation_ == Operation::connect;
      configured_ = active_;
      set_status(active_ ? "Connected." : "Disconnected.");
    } else if (operation_ == Operation::poll) {
      std::unique_ptr<xc_sync_network_events, decltype(&xc_sync_network_events_release)> events(
        xc_request_take_sync_network_events(finished.get()), xc_sync_network_events_release);
      active_ = events && xc_sync_network_events_active(events.get());
      if (!active_)
        set_status("Connection stopped. Reconnect to resume.");
      if (events && xc_sync_network_events_overflow(events.get()))
        set_status("Some received events were dropped.");
      if (active_)
        for (size_t i = 0; i < xc_sync_network_event_count(events.get()); ++i) {
          char kind = 0;
          xc_sync_navigation nav{};
          if (xc_sync_network_event_get(events.get(), i, &kind, &nav) == XC_OK && kind == 'N')
            incoming_.push_back({copy(nav.speaker), copy(nav.module), copy(nav.reference), copy(nav.group), copy(nav.domain)});
        }
    } else if (operation_ == Operation::plan) {
      std::unique_ptr<xc_sync_decision, decltype(&xc_sync_decision_release)> decision(
        xc_request_take_sync_decision(finished.get()), xc_sync_decision_release);
      xc_sync_result result{};
      if (decision && xc_sync_decision_get(decision.get(), &result) == XC_OK && result.route != 0) {
        const auto &nav = result.navigation;
        if (result.route == 1 && receive && receive(copy(nav.module), copy(nav.reference), result.tab_index, true)) {
          set_status("Opened received passage.");
        } else {
          if (review_.size() == 32)
            review_.erase(review_.begin());
          review_.push_back({copy(nav.speaker), copy(nav.module), copy(nav.reference), copy(nav.group), copy(nav.domain)});
          update_review();
          set_status("Received a passage for review.");
        }
      }
    } else {
      int sent = 0;
      xc_request_sync_transmitted(finished.get(), &sent);
      set_status(sent ? "Passage shared." : "Passage was not sent in the current mode.");
    }
  }
  if (active_ && !request_) {
    xc_request *request = nullptr;
    if (!incoming_.empty()) {
      auto item = std::move(incoming_.front());
      incoming_.pop_front();
      const xc_sync_navigation navigation{item.speaker.c_str(), item.module.c_str(), item.reference.c_str(), item.group.c_str(), item.domain.c_str()};
      auto status = xc_worker_plan_incoming_sync(&worker_, &navigation, &request);
      submit(status, request, Operation::plan);
    } else if (g_get_monotonic_time() >= next_poll_) {
      next_poll_ = g_get_monotonic_time() + 1050000;
      auto status = xc_worker_poll_sync_network(&worker_, &request);
      submit(status, request, Operation::poll);
    }
  }
  gtk_widget_set_sensitive(connect_, available_ && !active_ && !request_);
  gtk_widget_set_sensitive(disconnect_, configured_ && !request_);
  gtk_widget_set_sensitive(send_, active_ && !request_ && mode_applied_ != 3);
  gtk_widget_set_sensitive(review_button_, !review_.empty());
}
}
