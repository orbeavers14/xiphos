// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "session_model.hpp"
namespace xiphos::linux_preview
{
struct RecoveryDraft {
	std::string id, title, path, module, reference, saved, text, label;
	std::optional<std::string> revision;
	int target = 0;
	size_t start = 0, end = 0;
	bool generic = false;
};
// The host schedules and presents recovery; the shared core owns its format,
// validation, atomic replacement, previous fallback and revision checks.
class RecoveryModel
{
      public:
	explicit RecoveryModel(xc_worker &worker) : worker_(&worker) {}
	~RecoveryModel();
	bool load(const std::string &path);
	bool save(const std::string &path, const std::vector<WindowCapture> &windows,
		  const std::string &active, const std::vector<RecoveryDraft> &drafts);
	void tick();
	bool busy() const { return request_ != nullptr; }
	const std::string &error() const { return error_; }
	const std::vector<RecoveryDraft> &drafts() const { return drafts_; }
	bool previous() const { return previous_; }
	xc_native_session *copy_session() const;
	size_t session_windows() const { return session_windows_; }
	bool loaded() const { return loaded_; }

      private:
	enum class Operation { read,
			       capture,
			       save } operation_ = Operation::read;
	xc_recovery_snapshot *snapshot_ = nullptr;
	xc_worker *worker_;
	xc_request *request_ = nullptr;
	std::string path_, error_;
	std::optional<std::string> revision_;
	std::vector<RecoveryDraft> drafts_, pending_;
	size_t session_windows_ = 0;
	bool previous_ = false, loaded_ = false;
	bool accept(xc_status status, xc_request *request, Operation operation);
};
} // namespace xiphos::linux_preview
