// SPDX-License-Identifier: GPL-2.0-or-later
#include "recovery_model.hpp"
#include <memory>
namespace xiphos::linux_preview
{
namespace
{
std::string copy(const char *p) { return p ? p : ""; }
} // namespace
RecoveryModel::~RecoveryModel() { xc_request_release(request_); xc_recovery_snapshot_release(snapshot_); }
xc_native_session *RecoveryModel::copy_session() const {
 xc_native_session *session=nullptr;if(!snapshot_||xc_recovery_snapshot_copy_session(snapshot_,&session)!=XC_OK)return nullptr;return session;
}
bool RecoveryModel::accept(xc_status s, xc_request *r, Operation op)
{
	if (s != XC_OK || !r) {
		xc_request_release(r);
		error_ = "Recovery could not be submitted (status " + std::to_string(s) + ").";
		return false;
	}
	request_ = r;
	operation_ = op;
	error_.clear();
	return true;
}
bool RecoveryModel::load(const std::string &path)
{
	if (busy() || path.find('\0') != std::string::npos)
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_read_recovery(worker_, path.c_str(), &r);
	if (!accept(s, r, Operation::read))
		return false;
	path_ = path;
	return true;
}
bool RecoveryModel::save(const std::string &path, const std::vector<WindowCapture> &windows, const std::string &active, const std::vector<RecoveryDraft> &drafts)
{
	if (busy() || previous_ || path.find('\0') != std::string::npos || active.find('\0') != std::string::npos)
		return false;
	std::vector<xc_native_tab_capture> rows;
	for (const auto &w : windows) {
		if (!w.model || !w.model->ready() || w.model->busy() || w.key.find('\0') != std::string::npos || w.host.find('\0') != std::string::npos)
			return false;
		xc_native_tab_capture row{};
		xc_native_tab_capture_default(&row);
		row.tab = w.model->tab_id();
		row.key = w.key.c_str();
		row.host_state = w.host.c_str();
		row.parallel_showing = w.parallel_showing;
		row.commentary_showing = !w.model->passage().commentary.empty();
		rows.push_back(row);
	}
	xc_request *r = nullptr;
	auto s = xc_worker_capture_native_session(worker_, rows.empty() ? nullptr : rows.data(), rows.size(), active.empty() ? nullptr : active.c_str(), &r);
	if (!accept(s, r, Operation::capture))
		return false;
	path_ = path;
	pending_ = drafts;
	return true;
}
void RecoveryModel::tick()
{
	if (!request_)
		return;
	auto status = xc_request_poll(request_);
	if (status == XC_PENDING)
		return;
	std::unique_ptr<xc_request, decltype(&xc_request_release)> done(request_, xc_request_release);
	request_ = nullptr;
	if (status != XC_OK) {
		error_ = "Recovery file was retained; operation failed (status " + std::to_string(status) + ").";
		return;
	}
	if (operation_ == Operation::capture) {
		std::unique_ptr<xc_native_session, decltype(&xc_native_session_release)> session(xc_request_take_native_session(done.get()), xc_native_session_release);
		xc_recovery_snapshot *raw = nullptr;
		std::unique_ptr<xc_recovery_snapshot, decltype(&xc_recovery_snapshot_release)> snapshot(raw, xc_recovery_snapshot_release);
		// Non-annotation drafts must be passed together to preserve the shared limit.
		std::vector<xc_recovery_draft> rows;
		for (const auto &d : pending_)
			if (d.target != 3)
				rows.push_back({d.id.c_str(), d.title.c_str(), d.target, d.path.c_str(), d.module.c_str(), d.reference.c_str(), d.revision ? d.revision->c_str() : nullptr, d.saved.c_str(), d.text.c_str(), d.start, d.end});
		status = xc_recovery_snapshot_create(session.get(), rows.empty() ? nullptr : rows.data(), rows.size(), &raw);
		snapshot.reset(raw);
		if (status == XC_OK)
			for (const auto &d : pending_)
				if (d.target == 3) {
					xc_recovery_draft row{d.id.c_str(), d.title.c_str(), 3, d.path.c_str(), d.module.c_str(), d.reference.c_str(), d.revision ? d.revision->c_str() : nullptr, d.saved.c_str(), d.text.c_str(), d.start, d.end};
					status = xc_recovery_snapshot_append_annotation(snapshot.get(), &row, d.label.c_str());
					if (status != XC_OK)
						break;
				}
		if (status != XC_OK) {
			error_ = "Recovery draft validation failed; existing recovery retained.";
			return;
		}
		xc_request *r = nullptr;
		status = xc_worker_save_recovery(worker_, path_.c_str(), revision_ ? revision_->c_str() : nullptr, snapshot.get(), &r);
		accept(status, r, Operation::save);
		return;
	}
	std::unique_ptr<xc_recovery_snapshot, decltype(&xc_recovery_snapshot_release)> snapshot(xc_request_take_recovery_snapshot(done.get()), xc_recovery_snapshot_release);
	if (!snapshot) {
		error_ = "Missing recovery snapshot.";
		return;
	}
	revision_ = copy(xc_recovery_snapshot_revision(snapshot.get()));
	previous_ = xc_recovery_snapshot_from_previous(snapshot.get()) != 0;
	drafts_.clear();
	for (size_t i = 0; i < xc_recovery_snapshot_count(snapshot.get()); ++i) {
		xc_recovery_draft row{};
		if (xc_recovery_snapshot_draft(snapshot.get(), i, &row) != XC_OK) {
			error_ = "Invalid recovery draft.";
			return;
		}
		RecoveryDraft d;
		d.id = copy(row.id);
		d.title = copy(row.title);
		d.target = row.target;
		d.path = copy(row.path);
		d.module = copy(row.module);
		d.reference = copy(row.reference);
		if (row.origin_revision)
			d.revision = row.origin_revision;
		d.saved = copy(row.saved_text);
		d.text = copy(row.text);
		d.start = row.selection_start;
		d.end = row.selection_end;
		d.label = copy(xc_recovery_snapshot_annotation_label(snapshot.get(), i));
		d.generic = xc_recovery_snapshot_annotation_generic(snapshot.get(), i) != 0;
		drafts_.push_back(std::move(d));
	}
	xc_recovery_snapshot_release(snapshot_);snapshot_=snapshot.release();
 xc_native_session *session=copy_session();session_windows_=xc_native_session_tab_count(session);xc_native_session_release(session);
	loaded_ = true;
}
} // namespace xiphos::linux_preview
