// SPDX-License-Identifier: GPL-2.0-or-later
#include "map_ui.hpp"
#include "remote_io.hpp"
#include <webkit/webkit.h>
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace xiphos::linux_preview {
namespace {
namespace fs = std::filesystem;
using core::TransferControl;
using core::detail::TransferWorkspace;
constexpr size_t pack_limit = 128 * 1024 * 1024;
constexpr size_t read_limit = 8 * 1024 * 1024;

struct Archive {
  int fd;
  explicit Archive(const fs::path &path) : fd(open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK)) {
    struct stat info{};
    if (fd < 0 || fstat(fd, &info) || !S_ISREG(info.st_mode) || info.st_size < 127 || info.st_size > pack_limit) {
      if (fd >= 0)
        close(fd);
      throw std::runtime_error("Choose a regular PMTiles archive no larger than 128 MiB.");
    }
  }
  ~Archive() { close(fd); }
  Archive(const Archive &) = delete;
  std::string read(size_t offset, size_t length) const {
    std::string bytes(length, '\0');
    size_t filled = 0;
    while (filled < length) {
      const auto count = pread(fd, bytes.data() + filled, length - filled, offset + filled);
      if (count < 0 && errno == EINTR)
        continue;
      if (count < 0)
        throw std::runtime_error("Could not read the map archive.");
      if (!count)
        break;
      filled += count;
    }
    bytes.resize(filled);
    return bytes;
  }
};

struct Reply {
  WebKitScriptMessageReply *reply;
  JSCContext *context;
  Reply(WebKitScriptMessageReply *reply, JSCContext *context)
      : reply(webkit_script_message_reply_ref(reply)), context(JSC_CONTEXT(g_object_ref(context))) {}
  ~Reply() {
    g_object_unref(context);
    webkit_script_message_reply_unref(reply);
  }
  void error(const char *message) { webkit_script_message_reply_return_error_message(reply, message); }
  void send(JSCValue *value) {
    webkit_script_message_reply_return_value(reply, value);
    g_object_unref(value);
  }
  void boolean(bool value) { send(jsc_value_new_boolean(context, value)); }
  void text(const std::string &value) { send(jsc_value_new_string(context, value.c_str())); }
};

struct MapState {
  GtkWindow *window;
  GtkApplication *application;
  fs::path pack;
  std::shared_ptr<TransferWorkspace> stage;
  std::shared_ptr<Archive> installed;
  std::shared_ptr<TransferControl> transfer;
  bool closed = false, preparing = false;
  size_t read_bytes = 0;
  unsigned reads = 0;
  GtkNativeDialog *picker = nullptr;

  void close() {
    closed = true;
    if (transfer)
      transfer->request_cancel();
    if (picker) {
      gtk_native_dialog_destroy(picker);
      g_clear_object(&picker);
    }
  }
};

using State = std::shared_ptr<MapState>;
using Response = std::shared_ptr<Reply>;

void run(const State &state, std::function<std::string()> work,
         std::function<void(const std::string *, const char *)> done)
{
  struct Job {
    std::function<std::string()> work;
    std::function<void(const std::string *, const char *)> done;
  };
  auto *job = new Job{std::move(work), std::move(done)};
  auto *application = G_APPLICATION(state->application);
  g_application_hold(application);
  auto *task = g_task_new(application, nullptr, +[](GObject *source, GAsyncResult *result, gpointer) {
    auto *task = G_TASK(result);
    auto *job = static_cast<Job *>(g_task_get_task_data(task));
    GError *error = nullptr;
    std::unique_ptr<std::string> value(static_cast<std::string *>(g_task_propagate_pointer(task, &error)));
    job->done(value.get(), error ? error->message : nullptr);
    g_clear_error(&error);
    g_application_release(G_APPLICATION(source));
  }, nullptr);
  g_task_set_task_data(task, job, +[](gpointer data) { delete static_cast<Job *>(data); });
  g_task_run_in_thread(task, +[](GTask *task, gpointer, gpointer data, GCancellable *) {
    try {
      auto result = static_cast<Job *>(data)->work();
      g_task_return_pointer(task, new std::string(std::move(result)),
        +[](gpointer value) { delete static_cast<std::string *>(value); });
    } catch (const std::exception &error) {
      g_task_return_new_error(task, G_IO_ERROR, G_IO_ERROR_FAILED, "%s", error.what());
    }
  });
  g_object_unref(task);
}

void prepare(const State &state, const Response &reply, std::string input, std::string checksum)
{
  auto result = std::make_shared<std::shared_ptr<TransferWorkspace>>();
  auto control = std::make_shared<TransferControl>();
  state->transfer = control;
  state->preparing = true;
  run(state, [result, control, parent = state->pack.parent_path(), input = std::move(input), checksum = std::move(checksum)] {
    if (g_mkdir_with_parents(parent.c_str(), 0700) != 0)
      throw std::runtime_error("Could not create the map pack directory.");
    auto workspace = std::make_shared<TransferWorkspace>(parent.string());
    const auto staged = workspace->root() / "pack.pmtiles";
    if (!checksum.empty()) {
      core::TransferOptions options;
      options.workspace = workspace->root().string();
      options.maximum_download_bytes = pack_limit;
      options.remote_access_acknowledged = true;
      const auto download = core::detail::download_https(input, staged, options, *control);
      if (g_ascii_strcasecmp(download.sha256.c_str(), checksum.c_str()) != 0)
        throw std::runtime_error("Map pack checksum did not match.");
    } else {
      Archive source(input);
      std::ofstream output(staged, std::ios::binary);
      output.exceptions(std::ios::badbit | std::ios::failbit);
      size_t offset = 0;
      while (offset < pack_limit) {
        core::detail::TransferAccess::check(*control);
        auto bytes = source.read(offset, std::min<size_t>(64 * 1024, pack_limit - offset));
        if (bytes.empty())
          break;
        output.write(bytes.data(), bytes.size());
        offset += bytes.size();
      }
      if (!source.read(offset, 1).empty())
        throw std::runtime_error("Map pack exceeds 128 MiB.");
      output.close();
    }
    fs::permissions(staged, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
    Archive candidate(staged);
    if (candidate.read(0, 8) != "PMTiles\x03")
      throw std::runtime_error("Choose a version 3 PMTiles archive.");
    *result = std::move(workspace);
    return std::string{};
  }, [state, reply, result](const std::string *value, const char *error) {
    state->preparing = false;
    state->transfer.reset();
    if (state->closed)
      return;
    if (!value) {
      reply->error(error ? error : "Could not prepare the map pack.");
      return;
    }
    state->stage = std::move(*result);
    reply->boolean(true);
  });
}

struct Prompt {
  State state;
  Response reply;
  GtkWidget *url = nullptr, *checksum = nullptr;
};
void choose(const State &state, const Response &reply)
{
  auto *picker = gtk_file_chooser_native_new("Install offline map pack", state->window,
    GTK_FILE_CHOOSER_ACTION_OPEN, "Review pack", "Cancel");
  auto *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "PMTiles archives");
  gtk_file_filter_add_pattern(filter, "*.pmtiles");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(picker), filter);
  g_object_unref(filter);
  state->preparing = true;
  g_signal_connect_data(picker, "response", G_CALLBACK(+[](GtkNativeDialog *dialog, int response, gpointer data) {
    const auto &prompt = *static_cast<Prompt *>(data);
    prompt.state->preparing = false;
    if (!prompt.state->closed) {
      auto *file = response == GTK_RESPONSE_ACCEPT ? gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog)) : nullptr;
      auto *path = file ? g_file_get_path(file) : nullptr;
      if (path)
        prepare(prompt.state, prompt.reply, path, "");
      else
        prompt.reply->boolean(false);
      g_free(path);
      g_clear_object(&file);
    }
    prompt.state->picker = nullptr;
    g_object_unref(dialog);
  }), new Prompt{state, reply}, +[](gpointer data, GClosure *) { delete static_cast<Prompt *>(data); }, G_CONNECT_DEFAULT);
  state->picker = GTK_NATIVE_DIALOG(picker);
  gtk_native_dialog_show(state->picker);
}
void ask_download(const State &state, const Response &reply)
{
  auto *dialog = GTK_DIALOG(gtk_dialog_new_with_buttons("Download offline map pack", state->window,
    GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), "Cancel", GTK_RESPONSE_CANCEL,
    "Download and review", GTK_RESPONSE_ACCEPT, nullptr));
  auto *content = gtk_dialog_get_content_area(dialog);
  auto *url = gtk_entry_new();
  auto *checksum = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(url), 16384);
  gtk_entry_set_max_length(GTK_ENTRY(checksum), 64);
  gtk_entry_set_placeholder_text(GTK_ENTRY(url), "Publisher’s HTTPS URL");
  gtk_entry_set_placeholder_text(GTK_ENTRY(checksum), "Publisher’s SHA-256 checksum");
  gtk_accessible_update_property(GTK_ACCESSIBLE(url), GTK_ACCESSIBLE_PROPERTY_LABEL, "Publisher’s HTTPS URL", -1);
  gtk_accessible_update_property(GTK_ACCESSIBLE(checksum), GTK_ACCESSIBLE_PROPERTY_LABEL, "Publisher’s SHA-256 checksum", -1);
  gtk_box_append(GTK_BOX(content), url);
  gtk_box_append(GTK_BOX(content), checksum);
  state->preparing = true;
  g_signal_connect_data(dialog, "response", G_CALLBACK(+[](GtkDialog *dialog, int response, gpointer data) {
    const auto &prompt = *static_cast<Prompt *>(data);
    prompt.state->preparing = false;
    if (!prompt.state->closed) {
      if (response == GTK_RESPONSE_ACCEPT) {
        std::string url = gtk_editable_get_text(GTK_EDITABLE(prompt.url));
        std::string checksum = gtk_editable_get_text(GTK_EDITABLE(prompt.checksum));
        if (url.rfind("https://", 0) == 0 && checksum.size() == 64 &&
            std::all_of(checksum.begin(), checksum.end(), [](char c) { return g_ascii_isxdigit(c); }))
          prepare(prompt.state, prompt.reply, std::move(url), std::move(checksum));
        else
          prompt.reply->error("Enter an HTTPS URL and a 64-digit SHA-256 checksum.");
      } else {
        prompt.reply->boolean(false);
      }
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
  }), new Prompt{state, reply, url, checksum}, +[](gpointer data, GClosure *) { delete static_cast<Prompt *>(data); }, G_CONNECT_DEFAULT);
  gtk_window_present(GTK_WINDOW(dialog));
}

void dispatch(const State &state, JSCValue *message, const Response &reply)
{
  if (!jsc_value_is_object(message) || state->closed) {
    reply->error("Invalid map request.");
    return;
  }
  auto property = [&](const char *name) {
    return std::unique_ptr<JSCValue, decltype(&g_object_unref)>(jsc_value_object_get_property(message, name), g_object_unref);
  };
  auto action_value = property("action");
  auto *action_text = jsc_value_is_string(action_value.get()) ? jsc_value_to_string(action_value.get()) : nullptr;
  const std::string action = action_text ? action_text : "";
  g_free(action_text);
  if (action == "installed") {
    std::error_code error;
    reply->boolean(fs::is_regular_file(state->pack, error));
  } else if (action == "discard") {
    state->stage.reset();
    reply->boolean(true);
  } else if (action == "choose" || action == "download") {
    if (state->preparing)
      reply->error("A map pack is already being prepared.");
    else if (action == "choose")
      choose(state, reply);
    else
      ask_download(state, reply);
  } else if (action == "read") {
    auto offset = property("offset"), length = property("length"), staged = property("staged");
    const double from = jsc_value_is_number(offset.get()) ? jsc_value_to_double(offset.get()) : -1;
    const double count = jsc_value_is_number(length.get()) ? jsc_value_to_double(length.get()) : 0;
    if (!(from >= 0 && from <= pack_limit && count >= 1 && count <= read_limit &&
          std::floor(from) == from && std::floor(count) == count) || state->reads >= 32 ||
        count + state->read_bytes > 16 * 1024 * 1024) {
      reply->error("Invalid or excessive map archive reads.");
      return;
    }
    std::shared_ptr<Archive> archive;
    if (jsc_value_is_boolean(staged.get()) && jsc_value_to_boolean(staged.get())) {
      if (!state->stage) {
        reply->error("No staged map pack.");
        return;
      }
      archive = std::make_shared<Archive>(state->stage->root() / "pack.pmtiles");
    } else {
      if (!state->installed)
        state->installed = std::make_shared<Archive>(state->pack);
      archive = state->installed;
    }
    ++state->reads;
    state->read_bytes += size_t(count);
    run(state, [archive, from, count] {
      const auto bytes = archive->read(size_t(from), size_t(count));
      std::unique_ptr<gchar, decltype(&g_free)> encoded(
        g_base64_encode(reinterpret_cast<const guchar *>(bytes.data()), bytes.size()), g_free);
      return std::string(encoded.get());
    }, [state, reply, count](const std::string *value, const char *error) {
      --state->reads;
      state->read_bytes -= size_t(count);
      if (state->closed)
        return;
      if (value)
        reply->text(*value);
      else
        reply->error(error ? error : "Could not read the map pack.");
    });
  } else if (action == "install") {
    if (!state->stage || state->preparing || rename((state->stage->root() / "pack.pmtiles").c_str(), state->pack.c_str()))
      reply->error("Could not install the reviewed map pack.");
    else {
      state->stage.reset();
      state->installed.reset();
      reply->boolean(true);
    }
  } else if (action == "remove") {
    if (unlink(state->pack.c_str()) && errno != ENOENT)
      reply->error("Could not remove the installed map pack.");
    else {
      state->installed.reset();
      reply->boolean(true);
    }
  } else {
    reply->error("Unknown map request.");
  }
}
}

void open_map(GtkWindow *parent, bool software, const fs::path &assets, const fs::path &pack)
{
  auto *application = gtk_window_get_application(parent);
  auto *window = GTK_WINDOW(gtk_application_window_new(application));
  gtk_window_set_title(window, "Biblical Map");
  gtk_window_set_default_size(window, 900, 650);
  gtk_window_set_transient_for(window, parent);
  gtk_window_set_destroy_with_parent(window, TRUE);
  auto *session = webkit_network_session_new_ephemeral();
  auto *view = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW, "network-session", session, nullptr));
  g_object_unref(session);
  if (software)
    webkit_settings_set_hardware_acceleration_policy(webkit_web_view_get_settings(view), WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);
  auto *file = g_file_new_for_path((assets / "index.html").c_str());
  auto *uri = g_file_get_uri(file);
  g_object_unref(file);
  g_object_set_data_full(G_OBJECT(view), "map-entry-uri", uri, g_free);
  g_signal_connect(view, "decide-policy", G_CALLBACK(+[](WebKitWebView *view, WebKitPolicyDecision *decision,
      WebKitPolicyDecisionType type, gpointer) -> gboolean {
    if (type == WEBKIT_POLICY_DECISION_TYPE_RESPONSE)
      return FALSE;
    auto *navigation = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
    const auto *target = webkit_uri_request_get_uri(webkit_navigation_action_get_request(navigation));
    const auto *entry = static_cast<const char *>(g_object_get_data(G_OBJECT(view), "map-entry-uri"));
    if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION && g_strcmp0(target, entry) == 0)
      return FALSE;
    webkit_policy_decision_ignore(decision);
    return TRUE;
  }), nullptr);
  auto *state = new State(std::make_shared<MapState>(MapState{window, application, pack}));
  auto *manager = webkit_web_view_get_user_content_manager(view);
  g_signal_connect(manager, "script-message-with-reply-received::mapPack",
    G_CALLBACK(+[](WebKitUserContentManager *, JSCValue *message, WebKitScriptMessageReply *handle, gpointer data) -> gboolean {
      const auto &state = *static_cast<State *>(data);
      auto reply = std::make_shared<Reply>(handle, jsc_value_get_context(message));
      try {
        dispatch(state, message, reply);
      } catch (const std::exception &error) {
        reply->error(error.what());
      }
      return TRUE;
    }), state);
  webkit_user_content_manager_register_script_message_handler_with_reply(manager, "mapPack", nullptr);
  g_object_set_data_full(G_OBJECT(manager), "map-state", state, +[](gpointer data) {
    auto state = std::unique_ptr<State>(static_cast<State *>(data));
    (*state)->close();
  });
  g_signal_connect_data(window, "close-request", G_CALLBACK(+[](GtkWindow *, gpointer data) -> gboolean {
    (*static_cast<State *>(data))->close();
    return FALSE;
  }), new State(*state), +[](gpointer data, GClosure *) { delete static_cast<State *>(data); }, G_CONNECT_DEFAULT);
  gtk_window_set_child(window, GTK_WIDGET(view));
  webkit_web_view_load_uri(view, uri);
  gtk_window_present(window);
}
}
