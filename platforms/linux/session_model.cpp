// SPDX-License-Identifier: GPL-2.0-or-later
#include "session_model.hpp"
#include <memory>
namespace xiphos::linux_preview
{
namespace
{
std::string copy(const char *p) { return p ? p : ""; }
} // namespace
void ReaderModel::restore_next()
{
	if (!restoring_ || request_ || closed_)
		return;
	if (!error_.empty()) {
		restoring_.reset();
		return;
	}
	const auto saved = *restoring_;
	xc_request *r = nullptr;
	xc_status status;
	switch (restore_stage_++) {
	case 0:
		if (!saved.module.empty()) {
			const auto *m = module(saved.module);
			if (m && m->type == "Commentaries") {
				pending_module_ = saved.module;
				status = xc_worker_read_tab_entry(worker_, tab_, saved.module.c_str(), saved.reference.c_str(), &r);
				accept(status, r, Operation::entry);
			} else {
				status = xc_worker_read_passage(worker_, tab_, saved.module.c_str(), saved.commentary.c_str(), saved.reference.c_str(), &r);
				accept(status, r, Operation::passage);
			}
		}
		break;
	case 1:
		if (!saved.dictionary.empty()) {
			dictionary_source_module_ = saved.module;
			dictionary_source_reference_ = saved.reference;
			status = xc_worker_read_dictionary(worker_, tab_, saved.dictionary.c_str(), saved.dictionary_key.c_str(), &r);
			accept(status, r, Operation::dictionary);
		}
		break;
	case 2:
		status = xc_worker_book_document_selection(worker_, tab_, &r);
		accept(status, r, Operation::book_document);
		break;
	case 3:
		status = xc_worker_parallel_selection(worker_, tab_, &r);
		accept(status, r, Operation::restored_parallel);
		break;
	case 4:
		status = xc_worker_lexicon_preferences(worker_, tab_, &r);
		accept(status, r, Operation::lexicons);
		break;
	default:
		restoring_.reset();
		break;
	}
	if (!request_ && restoring_)
		restore_next();
}
SessionModel::~SessionModel()
{
	xc_request_release(request_);
	xc_native_session_release(saved_);
}
bool SessionModel::accept(xc_status status, xc_request *r, Operation operation)
{
	if (status != XC_OK || !r) {
		xc_request_release(r);
		error_ = "Session operation could not be submitted.";
		return false;
	}
	request_ = r;
	operation_ = operation;
	error_.clear();
	return true;
}
bool SessionModel::save(const std::string &path, const std::vector<WindowCapture> &windows, const std::string &active)
{
	if (busy() || path.find('\0') != std::string::npos || active.find('\0') != std::string::npos)
		return false;
	std::vector<xc_native_tab_capture> rows;
	rows.reserve(windows.size());
	for (const auto &window : windows) {
		if (window.key.find('\0') != std::string::npos || window.host.find('\0') != std::string::npos || !window.model || !window.model->ready() || window.model->busy() || !window.model->tab_id())
			return false;
		xc_native_tab_capture capture{};
		xc_native_tab_capture_default(&capture);
		capture.tab = window.model->tab_id();
		capture.key = window.key.c_str();
		capture.host_state = window.host.c_str();
		capture.commentary_showing = !window.model->passage().commentary.empty();
		capture.parallel_showing = window.parallel_showing;
		rows.push_back(capture);
	}
	xc_request *r = nullptr;
	auto status = xc_worker_save_native_session(worker_, path.c_str(), rows.empty() ? nullptr : rows.data(), rows.size(), active.empty() ? nullptr : active.c_str(), &r);
	return accept(status, r, Operation::save);
}
bool SessionModel::load(const std::string &path)
{
	if (busy() || path.find('\0') != std::string::npos)
		return false;
	xc_request *r = nullptr;
	auto status = xc_worker_read_native_session(worker_, path.c_str(), &r);
	return accept(status, r, Operation::read);
}
bool SessionModel::restore(const xc_native_session *snapshot)
{
 if(busy()||!snapshot)return false;
		staged_.clear();
	for (size_t i = 0; i < xc_native_session_tab_count(snapshot); ++i) {
		RestoredWindow w;
		w.key = copy(xc_native_session_tab_key(snapshot, i));
		w.module = copy(xc_native_session_field(snapshot, i, XC_SESSION_READING_MODULE));
		w.reference = copy(xc_native_session_field(snapshot, i, XC_SESSION_READING_REFERENCE));
		w.commentary = copy(xc_native_session_field(snapshot, i, XC_SESSION_READING_COMMENTARY));
		w.dictionary = copy(xc_native_session_field(snapshot, i, XC_SESSION_DICTIONARY_MODULE));
		w.dictionary_key = copy(xc_native_session_field(snapshot, i, XC_SESSION_DICTIONARY_KEY));
		w.host = copy(xc_native_session_field(snapshot, i, XC_SESSION_HOST_STATE));
		if (validate_host && !validate_host(w.host)) {
			error_ = "Unsupported saved window settings; no tabs were replaced.";
			return false;
		}
		staged_.push_back(std::move(w));
	}
	xc_session_capabilities caps{};
	xc_session_capabilities_default(&caps);
	caps.lexical_layout = caps.note_numbering = caps.heading_note_links = 1;
	xc_request *r = nullptr;
	auto s = xc_worker_restore_native_session(worker_, snapshot, &caps, &r);
	return accept(s, r, Operation::restore);
}
void SessionModel::tick()
{
	if (!request_)
		return;
	const auto status = xc_request_poll(request_);
	if (status == XC_PENDING)
		return;
	std::unique_ptr<xc_request, decltype(&xc_request_release)> done(request_, xc_request_release);
	request_ = nullptr;
	if (status != XC_OK) {
		error_ = "The session could not be saved/restored; the existing file was retained (status " + std::to_string(status) + ").";
		return;
	}
	if (operation_ == Operation::read) {
		xc_native_session_release(saved_);
		saved_ = xc_request_take_native_session(done.get());
		if (!saved_) {
			error_ = "Missing session snapshot.";
			return;
		}
		restore(saved_);
	} else if (operation_ == Operation::restore) {
		std::unique_ptr<xc_restored_session, decltype(&xc_restored_session_release)> p(xc_request_take_restored_session(done.get()), xc_restored_session_release);
		if (!p || xc_restored_session_tab_count(p.get()) != staged_.size()) {
			error_ = "Saved window mappings are inconsistent.";
			return;
		}
		for (size_t i = 0; i < staged_.size(); ++i) {
			if (copy(xc_restored_session_tab_key(p.get(), i)) != staged_[i].key) {
				error_ = "Saved window identity mismatch.";
				return;
			}
			staged_[i].tab = xc_restored_session_tab_id(p.get(), i);
			xc_tab_visibility visibility{};
			int commentary = 0, parallel = 0;
			xc_restored_session_visibility(p.get(), i, &visibility, &commentary, &parallel);
			staged_[i].parallel_showing = parallel != 0;
		}
		active_tab_ = xc_restored_session_active_tab(p.get());
		restored_ = std::move(staged_);
		++restored_version_;
		xc_native_session_release(saved_);
		saved_ = nullptr;
	} else {
		std::unique_ptr<xc_native_session, decltype(&xc_native_session_release)> saved(xc_request_take_native_session(done.get()), xc_native_session_release);
	}
}
} // namespace xiphos::linux_preview
