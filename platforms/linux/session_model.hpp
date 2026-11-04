// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "reader_model.hpp"
#include <functional>
namespace xiphos::linux_preview
{
struct WindowCapture {
	ReaderModel *model;
	std::string key, host;
	bool parallel_showing = false;
};
// One UI-thread observer for shared atomic capture/restore. It never owns an SDK.
class SessionModel
{
      public:
	explicit SessionModel(xc_worker &worker) : worker_(&worker) {}
	~SessionModel();
	bool save(const std::string &path, const std::vector<WindowCapture> &windows, const std::string &active);
	bool load(const std::string &path);
	bool restore(const xc_native_session *snapshot);
	void tick();
	bool busy() const { return request_ != nullptr; }
	const std::string &error() const { return error_; }
	const std::vector<RestoredWindow> &restored() const { return restored_; }
	uint64_t active_tab() const { return active_tab_; }
	unsigned restored_version() const { return restored_version_; }
	std::function<bool(const std::string &)> validate_host;

      private:
	enum class Operation { save,
			       read,
			       restore } operation_ = Operation::read;
	xc_worker *worker_;
	xc_request *request_ = nullptr;
	xc_native_session *saved_ = nullptr;
	std::vector<RestoredWindow> staged_, restored_;
	unsigned restored_version_ = 0;
	uint64_t active_tab_ = 0;
	std::string error_;
	bool accept(xc_status status, xc_request *request, Operation operation);
};
} // namespace xiphos::linux_preview
