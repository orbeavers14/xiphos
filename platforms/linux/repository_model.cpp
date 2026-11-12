// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_model.hpp"
#include "recovery_model.hpp"
#include <memory>
namespace xiphos::linux_preview
{
namespace
{
std::string copy(const char *p) { return p ? p : ""; }
bool safe(const std::string &s) { return s.find('\0') == std::string::npos; }
} // namespace
bool ReaderModel::open_bookmark(size_t index)
{
	if (!ready() || busy() || index >= bookmarks_.items.size())
		return false;
	const auto &item = bookmarks_.items[index];
	if (item.kind != XC_BOOKMARK_LEAF || !item.fields[XC_BOOKMARK_KEY])
		return false;
	const auto id = item.fields[XC_BOOKMARK_MODULE].value_or(passage_.bible);
	const auto *source = module(id);
	if (!source)
		return false;
	return activate_target({id, *item.fields[XC_BOOKMARK_KEY], 0, 0, source->dictionary ? 1 : source->book ? 2
													       : 0});
}
bool ReaderModel::open_bookmarks(const std::string &path, bool create)
{
	if (!ready() || busy() || !safe(path) || path.empty())
		return false;
	bookmark_import_.ready = false;
	if (create) {
		bookmarks_ = {path, std::nullopt, {}, bookmarks_.version + 1};
		return true;
	}
	xc_request *r = nullptr;
	auto s = xc_worker_read_bookmarks(worker_, tab_, path.c_str(), &r);
	if (!accept(s, r, Operation::bookmarks))
		return false;
	pending_repository_path_ = path;
	return true;
}
bool ReaderModel::edit_bookmark(int action, int kind, std::optional<size_t> target, std::optional<size_t> parent,
				size_t position, const std::array<std::optional<std::string>, 6> &fields)
{
	if (!ready() || busy() || bookmarks_.path.empty())
		return false;
	xc_bookmark_edit edit{};
	edit.action = action;
	edit.kind = kind;
	edit.has_target = target.has_value();
	edit.target = target.value_or(0);
	edit.has_parent = parent.has_value();
	edit.parent = parent.value_or(0);
	edit.position = position;
	for (size_t i = 0; i < fields.size(); ++i) {
		if (fields[i] && !safe(*fields[i]))
			return false;
		edit.fields[i] = fields[i] ? fields[i]->c_str() : nullptr;
	}
	xc_request *r = nullptr;
	auto s = xc_worker_edit_bookmarks(worker_, tab_, bookmarks_.path.c_str(), bookmarks_.revision ? bookmarks_.revision->c_str() : nullptr, &edit, &r);
	if (!accept(s, r, Operation::bookmarks))
		return false;
	pending_repository_path_ = bookmarks_.path;
	return true;
}
bool ReaderModel::preview_andbible(const std::string &path)
{
	if (!ready() || busy() || !safe(path) || bookmarks_.path.empty())
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_read_andbible_bookmarks(worker_, path.c_str(), &r);
	if (!accept(s, r, Operation::andbible))
		return false;
	bookmark_import_.ready = false;
	import_path_ = bookmarks_.path;
	import_revision_ = bookmarks_.revision;
	return true;
}
bool ReaderModel::merge_bookmarks(const std::string &folder, int policy)
{
	if (!ready() || busy() || !safe(folder) || !bookmark_import_.ready)
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_merge_bookmark_document(worker_, import_path_.c_str(), import_revision_ ? import_revision_->c_str() : nullptr, import_xml_.c_str(), folder.c_str(), policy, &r);
	if (!accept(s, r, Operation::bookmarks))
		return false;
	pending_repository_path_ = import_path_;
	bookmark_import_.ready = false;
	return true;
}
bool ReaderModel::export_bookmarks(const std::string &destination)
{
	if (!ready() || busy() || !safe(destination) || !bookmarks_.revision)
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_export_bookmark_document(worker_, bookmarks_.path.c_str(), bookmarks_.revision->c_str(), destination.c_str(), nullptr, &r);
	return accept(s, r, Operation::bookmarks_export);
}
bool ReaderModel::open_annotations(const std::string &path, bool create)
{
	if (!ready() || busy() || !safe(path) || path.empty())
		return false;
	if (create) {
		annotations_ = {path, std::nullopt, {}};
		return true;
	}
	xc_request *r = nullptr;
	auto s = xc_worker_read_annotations(worker_, tab_, path.c_str(), &r);
	if (!accept(s, r, Operation::annotations))
		return false;
	pending_repository_path_ = path;
	return true;
}
bool ReaderModel::begin_note(bool generic)
{
	if (!ready() || busy() || annotations_.path.empty() || passage_.bible.empty())
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_annotation_target(worker_, tab_, passage_.bible.c_str(), passage_.reference.c_str(), generic, &r);
	return accept(s, r, Operation::note_target);
}
bool ReaderModel::save_note(const std::string &text, bool remove)
{
	if (!ready() || busy() || note_draft_.path.empty() || !safe(text))
		return false;
	if (note_draft_.label.empty())
		return false;
	xc_annotation_edit edit{remove ? XC_ANNOTATION_REMOVE : XC_ANNOTATION_SAVE, note_draft_.label.c_str(), remove ? nullptr : text.c_str()};
	xc_request *r = nullptr;
	auto s = xc_worker_edit_annotations_checked(worker_, tab_, note_draft_.path.c_str(),
						    note_draft_.revision ? note_draft_.revision->c_str() : nullptr, &edit, &r);
	if (!accept(s, r, Operation::note_saved))
		return false;
	pending_repository_path_ = note_draft_.path;
	return true;
}
bool ReaderModel::decorate_reading(bool commentary)
{
	if (!ready() || busy() || passage_.bible.empty())
		return false;
	const auto &module = commentary ? passage_.commentary : passage_.bible;
	if (module.empty())
		return false;
	const char *marks = bookmarks_.path.empty() ? nullptr : bookmarks_.path.c_str();
	const char *notes = annotations_.path.empty() ? nullptr : annotations_.path.c_str();
	xc_request *r = nullptr;
	auto s = commentary
		     ? xc_worker_read_decorated_commentary(worker_, tab_, marks, notes, module.c_str(), passage_.reference.c_str(), show_commentary_chapter_, (display_flags_ & XC_CHAPTER_VERSE_NUMBERS) != 0, (display_flags_ & XC_CHAPTER_HIGHLIGHT_SELECTED) != 0, &r)
		     : xc_worker_read_decorated_reading(worker_, tab_, marks, notes, module.c_str(), passage_.reference.c_str(), show_chapter_, (display_flags_ & XC_CHAPTER_VERSE_NUMBERS) != 0, (display_flags_ & XC_CHAPTER_HIGHLIGHT_SELECTED) != 0, &r);
	if (!accept(s, r, Operation::decorated))
		return false;
	pending_decorated_commentary_ = commentary;
	return true;
}
bool ReaderModel::open_editor(const std::string &path, bool create)
{
	if (!ready() || busy() || editor_dirty() || !safe(path) || path.empty())
		return false;
	if (create) {
		xc_editor_buffer *buffer = nullptr;
		if (xc_editor_buffer_create("", &buffer) != XC_OK)
			return false;
		xc_editor_buffer_release(editor_buffer_);
		editor_buffer_ = buffer;
		editor_ = {path, "", "", "", std::nullopt, false, editor_.version + 1};
		return true;
	}
	xc_request *r = nullptr;
	auto s = xc_worker_read_editable_document(worker_, tab_, path.c_str(), &r);
	if (!accept(s, r, Operation::editor_file))
		return false;
	pending_editor_write_ = false;
	pending_editor_ = {path, "", "", "", std::nullopt};
	return true;
}
bool ReaderModel::open_module_editor(const std::string &module, const std::string &reference, bool new_section)
{
	if (!ready() || busy() || editor_dirty() || !safe(module) || !safe(reference))
		return false;
	xc_request *r = nullptr;
	auto s = new_section
		     ? xc_worker_prepare_book_entry(worker_, tab_, module.c_str(), reference.c_str(), &r)
		     : xc_worker_read_module_edit(worker_, tab_, module.c_str(), reference.c_str(), &r);
	if (!accept(s, r, Operation::editor_module))
		return false;
	pending_editor_write_ = false;
	pending_editor_ = {"", module, reference, "", std::nullopt, new_section};
	return true;
}
bool ReaderModel::change_editor(const std::string &text)
{
	if (!ready() || busy() || !editor_buffer_ || !safe(text))
		return false;
	if (xc_editor_buffer_replace(editor_buffer_, text.c_str()) != XC_OK)
		return false;
	editor_.text = copy(xc_editor_buffer_text(editor_buffer_));
	return true;
}
bool ReaderModel::editor_undo(bool redo)
{
	if (!ready() || busy() || !editor_buffer_)
		return false;
	const auto s = redo ? xc_editor_buffer_redo(editor_buffer_) : xc_editor_buffer_undo(editor_buffer_);
	if (s != XC_OK)
		return false;
	editor_.text = copy(xc_editor_buffer_text(editor_buffer_));
	++editor_.version;
	return true;
}
std::string ReaderModel::editor_saved_text() const { return copy(xc_editor_buffer_saved_text(editor_buffer_)); }
bool ReaderModel::recover_draft(const RecoveryDraft &d)
{
	if (!ready() || busy() || (d.target != 3 && editor_dirty()))
		return false;
	if (d.target == 3) {
		note_draft_ = {d.path, d.module, d.reference, d.label, d.saved, d.revision, d.generic, note_draft_.version + 1};
		return true;
	}
	xc_editor_buffer *buffer = nullptr;
	if (xc_editor_buffer_create(d.saved.c_str(), &buffer) != XC_OK)
		return false;
	if (xc_editor_buffer_replace(buffer, d.text.c_str()) != XC_OK) {
		xc_editor_buffer_release(buffer);
		return false;
	}
	xc_editor_buffer_release(editor_buffer_);
	editor_buffer_ = buffer;
	editor_ = {d.path, d.module, d.reference, d.text, d.revision, false, editor_.version + 1};
	xc_request *r = nullptr;
	if (d.target == 2) {
		auto s = xc_worker_read_module_edit(worker_, tab_, d.module.c_str(), d.reference.c_str(), &r);
		return accept(s, r, Operation::editor_bind);
	}
	auto s = xc_worker_release_module_editor(worker_, tab_, &r);
	return accept(s, r, Operation::editor_release);
}
bool ReaderModel::editor_dirty() const { return editor_buffer_ && xc_editor_buffer_dirty(editor_buffer_); }
bool ReaderModel::discard_editor()
{
	if (!ready() || busy() || !editor_buffer_)
		return false;
	return change_editor(editor_saved_text()) && (++editor_.version, true);
}
bool ReaderModel::save_editor_as(const std::string &path)
{
	if (!ready() || busy() || !editor_buffer_ || !safe(path) || path.empty())
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_save_editable_document(worker_, tab_, path.c_str(), nullptr, editor_.text.c_str(), &r);
	if (!accept(s, r, Operation::editor_file))
		return false;
	pending_editor_ = {path, "", "", editor_.text, std::nullopt, false};
	return true;
}
bool ReaderModel::save_editor(bool erase)
{
	if (!ready() || busy() || !editor_buffer_)
		return false;
	xc_request *r = nullptr;
	xc_status s;
	if (editor_.module.empty()) {
		if (erase)
			return false;
		s = xc_worker_save_editable_document(worker_, tab_, editor_.path.c_str(), editor_.revision ? editor_.revision->c_str() : nullptr, editor_.text.c_str(), &r);
	} else if (editor_.new_section)
		s = xc_worker_create_book_entry(worker_, tab_, editor_.module.c_str(), editor_.reference.c_str(), editor_.revision ? editor_.revision->c_str() : nullptr, editor_.text.c_str(), &r);
	else
		s = xc_worker_save_module_edit(worker_, tab_, editor_.module.c_str(), editor_.reference.c_str(), editor_.revision ? editor_.revision->c_str() : nullptr, editor_.text.c_str(), erase, &r);
	if (!accept(s, r, editor_.module.empty() ? Operation::editor_file : Operation::editor_module))
		return false;
	pending_editor_write_ = !editor_.module.empty();
	pending_editor_ = editor_;
	pending_editor_.new_section = false;
	return true;
}
bool ReaderModel::organize_book_entry(int action, const std::string &destination)
{
	if (!ready() || busy() || editor_dirty() || editor_.new_section ||
	    !editor_.revision || editor_.module.empty() || !safe(destination))
		return false;
	const auto *module = this->module(editor_.module);
	if (!module || !module->book)
		return false;
	const auto *id = editor_.module.c_str(), *path = editor_.reference.c_str(), *revision = editor_.revision->c_str();
	xc_request *request = nullptr;
	xc_status status;
	switch (action) {
	case 0:
		status = xc_worker_rename_book_leaf(worker_, tab_, id, path, destination.c_str(), revision, &request);
		break;
	case 1:
		status = xc_worker_rename_book_subtree(worker_, tab_, id, path, destination.c_str(), revision, &request);
		break;
	case 2:
		status = xc_worker_move_book_subtree(worker_, tab_, id, path, destination.c_str(), revision, &request);
		break;
	case 3:
		status = xc_worker_delete_book_leaf(worker_, tab_, id, path, revision, &request);
		break;
	case 4:
		status = xc_worker_delete_book_subtree(worker_, tab_, id, path, revision, &request);
		break;
	default:
		return false;
	}
	if (!accept(status, request, Operation::editor_module))
		return false;
	pending_editor_ = editor_;
	pending_editor_write_ = true;
	return true;
}
bool ReaderModel::format_editor(size_t start, size_t length, int action, int value, const std::string &text)
{
	if (!ready() || busy() || !editor_buffer_ || !safe(text))
		return false;
	std::unique_ptr<xc_rich_document, decltype(&xc_rich_document_release)> parsed(nullptr, xc_rich_document_release), formatted(nullptr, xc_rich_document_release);
	xc_rich_document *p = nullptr;
	if (xc_rich_document_parse(editor_.text.c_str(), &p) != XC_OK)
		return false;
	parsed.reset(p);
	p = nullptr;
	if (xc_rich_document_format(parsed.get(), start, length, action, value, text.c_str(), &p) != XC_OK)
		return false;
	formatted.reset(p);
	if (!change_editor(copy(xc_rich_document_html(formatted.get()))))
		return false;
	++editor_.version;
	return true;
}
bool ReaderModel::take_repository(xc_request *r)
{
	switch (operation_) {
	case Operation::editor_bind: {
		std::unique_ptr<xc_module_edit, decltype(&xc_module_edit_release)> p(xc_request_take_module_edit(r), xc_module_edit_release);
		if (!p)
			error_ = "Recovered module draft is retained but the source could not be opened.";
		return true;
	}
	case Operation::editor_release:
		return true;
	case Operation::andbible: {
		std::unique_ptr<xc_andbible_import, decltype(&xc_andbible_import_release)> p(xc_request_take_andbible_import(r), xc_andbible_import_release);
		if (!p) {
			error_ = "Missing AndBible preview.";
			return true;
		}
		import_xml_ = copy(xc_andbible_import_xml(p.get()));
		xc_andbible_import_counts(p.get(), &bookmark_import_.source);
		xc_request *next = nullptr;
		auto s = xc_worker_review_bookmark_import(worker_, import_path_.c_str(), import_revision_ ? import_revision_->c_str() : nullptr, import_xml_.c_str(), &next);
		accept(s, next, Operation::import_review);
		return true;
	}
	case Operation::import_review: {
		std::unique_ptr<xc_bookmark_import_review, decltype(&xc_bookmark_import_review_release)> p(xc_request_take_bookmark_import_review(r), xc_bookmark_import_review_release);
		if (!p) {
			error_ = "Missing bookmark collision review.";
			return true;
		}
		xc_bookmark_import_review_counts(p.get(), &bookmark_import_.review);
		bookmark_import_.collisions.clear();
		for (size_t i = 0; i < bookmark_import_.review.collision_count; ++i) {
			xc_bookmark_import_collision row{};
			if (xc_bookmark_import_review_collision(p.get(), i, &row) == XC_OK)
				bookmark_import_.collisions.push_back(copy(row.identity) + " · " + copy(row.key) + "\nExisting: " + copy(row.existing_description) + "\nIncoming: " + copy(row.incoming_description));
		}
		bookmark_import_.ready = true;
		++bookmark_import_.version;
		return true;
	}
	case Operation::bookmarks_export: {
		std::unique_ptr<xc_bookmarks, decltype(&xc_bookmarks_release)> p(xc_request_take_bookmarks(r), xc_bookmarks_release);
		if (p)
			notice_ = "Exact bookmark XML exported.";
		else
			error_ = "Missing exported bookmark result.";
		return true;
	}
	case Operation::bookmarks: {
		std::unique_ptr<xc_bookmarks, decltype(&xc_bookmarks_release)> p(xc_request_take_bookmarks(r), xc_bookmarks_release);
		if (!p) {
			error_ = "Missing bookmark snapshot.";
			return true;
		}
		BookmarkRepository v{pending_repository_path_, std::nullopt, {}, bookmarks_.version + 1};
		if (auto *revision = xc_bookmarks_revision(p.get()))
			v.revision = revision;
		for (size_t i = 0; i < xc_bookmarks_count(p.get()); ++i) {
			BookmarkItem item;
			item.kind = xc_bookmarks_kind(p.get(), i);
			item.position = xc_bookmarks_position(p.get(), i);
			int present = 0;
			size_t parent = 0;
			xc_bookmarks_parent(p.get(), i, &present, &parent);
			if (present)
				item.parent = parent;
			for (int f = 0; f < 8; ++f)
				if (auto *s = xc_bookmarks_field(p.get(), i, f))
					item.fields[f] = s;
			v.items.push_back(std::move(item));
		}
		bookmarks_ = std::move(v);
		return true;
	}
	case Operation::annotations:
	case Operation::note_saved: {
		std::unique_ptr<xc_annotations, decltype(&xc_annotations_release)> p(xc_request_take_annotations(r), xc_annotations_release);
		if (!p) {
			error_ = "Missing annotation snapshot.";
			return true;
		}
		AnnotationRepository v{pending_repository_path_, std::nullopt, {}};
		if (auto *revision = xc_annotations_revision(p.get()))
			v.revision = revision;
		for (size_t i = 0; i < xc_annotations_count(p.get()); ++i)
			v.items.push_back({copy(xc_annotations_field(p.get(), i, XC_ANNOTATION_LABEL)), copy(xc_annotations_field(p.get(), i, XC_ANNOTATION_NOTE)), xc_annotations_legacy_visible(p.get(), i) != 0});
		annotations_ = std::move(v);
		if (operation_ == Operation::note_saved) {
			note_draft_.revision = annotations_.revision;
			note_draft_.text.clear();
			for (const auto &item : annotations_.items)
				if (item.label == note_draft_.label) {
					note_draft_.text = item.note;
					break;
				}
			++note_draft_.version;
		}
		return true;
	}
	case Operation::note_target: {
		std::unique_ptr<xc_annotation_target, decltype(&xc_annotation_target_release)> p(xc_request_take_annotation_target(r), xc_annotation_target_release);
		if (!p) {
			error_ = "Missing verse note target.";
			return true;
		}
		NoteDraft v{annotations_.path, copy(xc_annotation_target_control_module(p.get())), copy(xc_annotation_target_reference(p.get())), copy(xc_annotation_target_label(p.get())), "", annotations_.revision, xc_annotation_target_generic(p.get()) != 0, note_draft_.version + 1};
		for (const auto &item : annotations_.items)
			if (item.label == v.label) {
				v.text = item.note;
				break;
			}
		note_draft_ = std::move(v);
		return true;
	}
	case Operation::editor_file:
	case Operation::editor_module: {
		EditorDraft v = pending_editor_;
		if (operation_ == Operation::editor_file) {
			std::unique_ptr<xc_editable_document, decltype(&xc_editable_document_release)> p(xc_request_take_editable_document(r), xc_editable_document_release);
			if (!p) {
				error_ = "Missing editor file.";
				return true;
			}
			v.text = copy(xc_editable_document_text(p.get()));
			if (auto *rev = xc_editable_document_revision(p.get()))
				v.revision = rev;
		} else {
			std::unique_ptr<xc_module_edit, decltype(&xc_module_edit_release)> p(xc_request_take_module_edit(r), xc_module_edit_release);
			if (!p) {
				error_ = "Missing module editor source.";
				return true;
			}
			v.module = copy(xc_module_edit_module(p.get()));
			v.reference = copy(xc_module_edit_reference(p.get()));
			v.text = copy(xc_module_edit_text(p.get()));
			if (auto *rev = xc_module_edit_revision(p.get()))
				v.revision = rev;
		}
		xc_editor_buffer *buffer = nullptr;
		if (xc_editor_buffer_create(v.text.c_str(), &buffer) != XC_OK) {
			error_ = "Could not prepare editor buffer.";
			return true;
		}
		xc_editor_buffer_release(editor_buffer_);
		editor_buffer_ = buffer;
		v.version = editor_.version + 1;
		editor_ = std::move(v);
		if (operation_ == Operation::editor_module && pending_editor_write_) {
			++content_version_;
			pending_editor_write_ = false;
		}
		if (operation_ == Operation::editor_file) {
			xc_request *next = nullptr;
			auto status = xc_worker_release_module_editor(worker_, tab_, &next);
			accept(status, next, Operation::editor_release);
		}
		return true;
	}
	case Operation::decorated: {
		std::unique_ptr<xc_decorated_reading, decltype(&xc_decorated_reading_release)> p(xc_request_take_decorated_reading(r), xc_decorated_reading_release);
		if (!p) {
			error_ = "Missing decorated reading.";
			return true;
		}
		const size_t i = pending_decorated_commentary_ ? 1 : 0;
		decorated_[i] = {copy(xc_decorated_reading_html(p.get())), copy(xc_decorated_reading_css(p.get())), pending_decorated_commentary_ ? passage_.commentary : passage_.bible, passage_.reference, pending_decorated_commentary_, pending_decorated_commentary_ ? show_commentary_chapter_ : show_chapter_, decorated_[i].version + 1};
		return true;
	}
	default:
		return false;
	}
}
} // namespace xiphos::linux_preview
