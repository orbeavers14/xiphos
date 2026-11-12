// SPDX-License-Identifier: GPL-2.0-or-later
#include "library_model.hpp"
#include <ctime>
#include <memory>
namespace xiphos::linux_preview
{
namespace
{
std::string copy(const char *p) { return p ? p : ""; }
bool safe(const std::string &s) { return s.find('\0') == std::string::npos; }
} // namespace
LibraryModel::~LibraryModel()
{
	xc_legacy_migration_release(migration_);
	xc_legacy_assets_release(assets_);
	xc_legacy_repository_preview_release(legacy_);
	xc_request_release(request_);
	xc_transfer_control_release(control_);
	xc_repository_snapshot_release(repositories_);
	xc_remote_catalog_release(remote_);
	xc_remote_install_result_release(remote_result_);
}
bool LibraryModel::accept(xc_status s, xc_request *r, Operation op)
{
	if (s != XC_OK || !r) {
		xc_request_release(r);
		error_ = "Library operation could not be submitted (status " + std::to_string(s) + ").";
		return false;
	}
	request_ = r;
	operation_ = op;
	error_.clear();
	notice_.clear();
	xc_transfer_control_release(control_);
	control_ = xc_request_take_transfer_control(r);
	progress_ = {};
	++version_;
	return true;
}
bool LibraryModel::catalog(const std::string &root)
{
	if (busy() || !safe(root))
		return false;
	xc_request *r = nullptr;
	auto s = root.empty() ? xc_worker_library_catalog(worker_, &r) : xc_worker_local_library_catalog(worker_, root.c_str(), &r);
	return accept(s, r, Operation::catalog);
}
bool LibraryModel::prepare_install(const std::string &root, const std::string &module)
{
	if (busy() || !safe(root) || !safe(module))
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_prepare_local_install(worker_, root.c_str(), module.c_str(), &r);
	if (!accept(s, r, Operation::prepare))
		return false;
	source_root_ = root;
	install_ = {};
	return true;
}
bool LibraryModel::install(bool replace)
{
	if (busy() || install_.module.empty())
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_install_local_module_task(worker_, source_root_.c_str(), install_.module.c_str(), install_.source.c_str(), install_.destination.c_str(), replace, &r);
	return accept(s, r, Operation::install);
}
bool LibraryModel::create_module(const std::string &module, unsigned kind)
{
	if (busy() || module.empty() || !safe(module) || kind > 2)
		return false;
	xc_request *request = nullptr;
	const auto create = kind == 0 ? xc_worker_create_general_book :
		kind == 1 ? xc_worker_create_personal_commentary : xc_worker_create_study_organizer;
	auto status = create(worker_, module.c_str(), &request);
	return accept(status, request, Operation::create_module);
}
bool LibraryModel::archive_module(const std::string &module, const std::string &path)
{
	if (busy() || module.empty() || path.empty() || !safe(module) || !safe(path))
		return false;
	xc_request *request = nullptr;
	auto status = xc_worker_archive_module(worker_, module.c_str(), path.c_str(), &request);
	return accept(status, request, Operation::archive_module);
}
bool LibraryModel::review_maintenance(const std::string &module)
{
	if (busy() || !safe(module))
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_preview_module_maintenance(worker_, module.c_str(), &r);
	if (!accept(s, r, Operation::maintenance))
		return false;
	maintenance_ = {};
	return true;
}
bool LibraryModel::maintain(int action, const std::string &key)
{
	if (busy() || maintenance_.module.empty() || !safe(key))
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_maintain_module_task(worker_, maintenance_.module.c_str(), maintenance_.revision.c_str(), action, key.c_str(), &r);
	return accept(s, r, Operation::maintain);
}
bool LibraryModel::repositories(const std::string &path, bool create)
{
	if (busy() || !safe(path) || path.empty())
		return false;
	if (create) {
		xc_repository_snapshot *p = nullptr;
		if (xc_repository_snapshot_create(&p) != XC_OK)
			return false;
		xc_repository_snapshot_release(repositories_);
		repositories_ = p;
		repository_path_ = path;
		repository_revision_.reset();
		remote_source_id_.clear();
		copy_repositories();
		return true;
	}
	xc_request *r = nullptr;
	auto s = xc_worker_read_repositories(worker_, path.c_str(), &r);
	if (!accept(s, r, Operation::repositories))
		return false;
	repository_path_ = path;
	return true;
}
bool LibraryModel::put_repository(const LibraryEndpoint &e)
{
	if (busy() || !repositories_ || !safe(e.id) || !safe(e.host) || !safe(e.catalog) || !safe(e.packages) || !safe(e.name))
		return false;
	xc_https_repository row{e.id.c_str(), e.host.c_str(), e.catalog.c_str(), e.packages.c_str(), e.port};
	auto s = xc_repository_snapshot_put(repositories_, &row, e.name.c_str(), e.enabled);
	if (s != XC_OK) {
		error_ = "Repository fields were rejected by the shared validator.";
		return false;
	}
	remote_source_id_.clear();
	copy_repositories();
	return true;
}
bool LibraryModel::remove_repository(const std::string &id)
{
	if (busy() || !repositories_ || !safe(id))
		return false;
	if (xc_repository_snapshot_remove(repositories_, id.c_str()) != XC_OK)
		return false;
	remote_source_id_.clear();
	copy_repositories();
	return true;
}
bool LibraryModel::move_repository(const std::string &id, size_t position)
{
	if (busy() || !repositories_ || !safe(id))
		return false;
	if (xc_repository_snapshot_move(repositories_, id.c_str(), position) != XC_OK)
		return false;
	copy_repositories();
	return true;
}
bool LibraryModel::save_repositories()
{
	if (busy() || !repositories_)
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_save_repositories(worker_, repository_path_.c_str(), repository_revision_ ? repository_revision_->c_str() : nullptr, repositories_, &r);
	return accept(s, r, Operation::save_repositories);
}
const xc_remote_catalog *LibraryModel::remote_catalog() const { return remote_result_ ? xc_remote_install_result_next_catalog(remote_result_) : remote_; }
bool LibraryModel::fetch(const std::string &id, const std::string &workspace, bool acknowledged, const std::string &ca_bundle)
{
	if (busy() || !safe(id) || !safe(workspace) || !safe(ca_bundle))
		return false;
	const LibraryEndpoint *found = nullptr;
	for (const auto &e : endpoints_)
		if (e.id == id)
			found = &e;
	if (!found || !found->enabled) {
		error_ = "Select an enabled reviewed source.";
		return false;
	}
	xc_https_repository row{found->id.c_str(), found->host.c_str(), found->catalog.c_str(), found->packages.c_str(), found->port};
	xc_transfer_options options{};
	xc_transfer_options_default(&options);
	options.workspace = workspace.c_str();
	options.ca_bundle = ca_bundle.empty() ? nullptr : ca_bundle.c_str();
	options.remote_access_acknowledged = acknowledged;
	xc_request *r = nullptr;
	auto s = xc_worker_fetch_remote_catalog(worker_, &row, &options, &r);
	if (!accept(s, r, Operation::fetch))
		return false;
	remote_source_id_ = id;
	return true;
}
bool LibraryModel::use_cached(size_t index)
{
	if (busy() || !repositories_ || index >= endpoints_.size())
		return false;
	xc_remote_catalog *p = nullptr;
	if (xc_repository_snapshot_copy_catalog(repositories_, index, &p) != XC_OK) {
		error_ = "No usable cached catalog for this source.";
		return false;
	}
	xc_remote_catalog_release(remote_);
	remote_ = p;
	xc_remote_install_result_release(remote_result_);
	remote_result_ = nullptr;
	remote_source_id_ = endpoints_[index].id;
	copy_catalog(xc_remote_catalog_library(remote_));
	return true;
}
bool LibraryModel::install_remote(const std::string &module, const std::string &workspace, bool replace, bool acknowledged, const std::string &ca_bundle)
{
	if (busy() || !remote_catalog() || !safe(module) || !safe(workspace) || !safe(ca_bundle))
		return false;
	bool enabled = false;
	for (const auto &e : endpoints_)
		if (e.id == remote_source_id_)
			enabled = e.enabled;
	if (!enabled) {
		error_ = "Enable the reviewed source before installing.";
		return false;
	}
	xc_transfer_options options{};
	xc_transfer_options_default(&options);
	options.workspace = workspace.c_str();
	options.ca_bundle = ca_bundle.empty() ? nullptr : ca_bundle.c_str();
	options.remote_access_acknowledged = acknowledged;
	xc_request *r = nullptr;
	auto s = xc_worker_install_remote_module(worker_, remote_catalog(), module.c_str(), replace, &options, &r);
	return accept(s, r, Operation::remote_install);
}
bool LibraryModel::prepare_migration(const std::string &settings, const std::string &tabs, const std::string &options, const std::vector<std::pair<std::string, std::string>> &mapping)
{
	if (busy() || !safe(settings) || !safe(tabs) || !safe(options))
		return false;
	std::vector<xc_legacy_module_mapping> rows;
	for (const auto &m : mapping) {
		if (!safe(m.first) || !safe(m.second))
			return false;
		rows.push_back({m.first.c_str(), m.second.c_str()});
	}
	xc_legacy_migration_input input{settings.empty() ? nullptr : settings.c_str(), tabs.empty() ? nullptr : tabs.c_str(), options.empty() ? nullptr : options.c_str()};
	xc_request *r = nullptr;
	auto s = xc_worker_prepare_legacy_migration(worker_, &input, rows.empty() ? nullptr : rows.data(), rows.size(), &r);
	return accept(s, r, Operation::migration);
}
bool LibraryModel::read_migration(const std::string &path, const std::vector<std::pair<std::string, std::string>> &mapping)
{
	if (busy() || !safe(path))
		return false;
	std::vector<xc_legacy_module_mapping> rows;
	for (const auto &m : mapping) {
		if (!safe(m.first) || !safe(m.second))
			return false;
		rows.push_back({m.first.c_str(), m.second.c_str()});
	}
	xc_request *r = nullptr;
	auto s = xc_worker_read_legacy_migration(worker_, path.c_str(), rows.empty() ? nullptr : rows.data(), rows.size(), &r);
	return accept(s, r, Operation::migration);
}
bool LibraryModel::write_migration(const std::string &destination, int action)
{
	if (busy() || !migration_ || !safe(destination) || action < 0 || action > 2)
		return false;
	xc_request *r = nullptr;
	auto s = action == 0 ? xc_worker_save_legacy_migration(worker_, destination.c_str(), migration_, &r) : action == 1 ? xc_worker_export_legacy_migration_session(worker_, destination.c_str(), migration_, &r)
															   : xc_worker_restore_legacy_migration_sources(worker_, destination.c_str(), migration_, &r);
	return accept(s, r, Operation::migration_write);
}
bool LibraryModel::inspect_assets(const std::string &source)
{
	if (busy() || !safe(source))
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_inspect_legacy_assets_task(worker_, source.c_str(), &r);
	return accept(s, r, Operation::assets);
}
bool LibraryModel::copy_assets(const std::string &destination)
{
	if (busy() || !assets_ || !safe(destination))
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_copy_legacy_assets_task(worker_, xc_legacy_assets_source(assets_), xc_legacy_assets_revision(assets_), destination.c_str(), &r);
	return accept(s, r, Operation::assets_copy);
}
bool LibraryModel::preview_legacy_sources(const std::string &path)
{
	if (busy() || !safe(path))
		return false;
	xc_request *r = nullptr;
	auto s = xc_worker_preview_legacy_repositories(worker_, path.c_str(), &r);
	return accept(s, r, Operation::legacy_sources);
}
bool LibraryModel::accept_legacy_source(size_t index, const std::string &id, const std::string &packages)
{
	if (busy() || !repositories_ || !legacy_ || !safe(id) || !safe(packages))
		return false;
	if (xc_repository_snapshot_accept_legacy(repositories_, legacy_, index, id.c_str(), packages.c_str()) != XC_OK) {
		error_ = "Legacy source is unsuitable or needs explicit safe endpoint fields.";
		return false;
	}
	copy_repositories();
	notice_ = "Reviewed source added disabled. Enable and save it explicitly before network access.";
	return true;
}
bool LibraryModel::organization(const std::string &path, bool create)
{
	if (busy() || !safe(path) || path.empty())
		return false;
	if (create) {
		organization_path_ = path;
		organization_revision_.reset();
		groups_.clear();
		++version_;
		return true;
	}
	xc_request *r = nullptr;
	auto s = xc_worker_read_library_organization(worker_, path.c_str(), &r);
	if (!accept(s, r, Operation::organization))
		return false;
	organization_path_ = path;
	return true;
}
bool LibraryModel::organize(const LibraryOrganization &e)
{
	if (busy() || organization_path_.empty() || !safe(e.provider) || !safe(e.item) || !safe(e.group))
		return false;
	xc_library_organization_entry row{e.provider.c_str(), e.item.c_str(), e.group.c_str(), e.favorite, e.hidden};
	xc_request *r = nullptr;
	auto s = xc_worker_set_library_organization(worker_, organization_path_.c_str(), organization_revision_ ? organization_revision_->c_str() : nullptr, &row, &r);
	return accept(s, r, Operation::organize);
}
bool LibraryModel::cancel() { return control_ && xc_transfer_control_cancel(control_); }
void LibraryModel::copy_catalog(const xc_library_catalog *catalog)
{
	items_.clear();
	for (size_t i = 0; i < xc_library_catalog_count(catalog); ++i) {
		xc_library_item row{};
		if (xc_library_catalog_item(catalog, i, &row) != XC_OK)
			continue;
		items_.push_back({copy(row.provider), copy(row.item), copy(row.title), copy(row.language), copy(row.type), copy(row.version), copy(row.license), row.access, row.capabilities, row.installed != 0});
	}
	++version_;
}
void LibraryModel::copy_repositories()
{
	endpoints_.clear();
	for (size_t i = 0; i < xc_repository_snapshot_count(repositories_); ++i) {
		xc_saved_repository row{};
		if (xc_repository_snapshot_get(repositories_, i, &row) == XC_OK)
			endpoints_.push_back({copy(row.id), copy(row.host), copy(row.catalog_directory), copy(row.package_directory), copy(row.name), row.port, row.enabled != 0, row.cached != 0, row.fetched_at});
	}
	++version_;
}
void LibraryModel::tick()
{
	if (!request_)
		return;
	if (control_)
		xc_transfer_control_progress(control_, &progress_);
	auto s = xc_request_poll(request_);
	if (s == XC_PENDING)
		return;
	std::unique_ptr<xc_request, decltype(&xc_request_release)> done(request_, xc_request_release);
	request_ = nullptr;
	xc_transfer_control_release(control_);
	control_ = nullptr;
	++version_;
	if (s != XC_OK) {
		error_ = "Library operation failed (status " + std::to_string(s) + "). Reviewed choices and existing files were retained.";
		return;
	}
	switch (operation_) {
	case Operation::create_module:
		++changed_;
		notice_ = "Module created. Refresh Installed modules to see it here.";
		break;
	case Operation::archive_module:
		notice_ = "Module archive saved.";
		break;
	case Operation::migration: {
		auto *p = xc_request_take_legacy_migration(done.get());
		if (!p) {
			error_ = "Missing migration review.";
			break;
		}
		xc_legacy_migration_release(migration_);
		migration_ = p;
		migration_review_ = std::string(xc_legacy_migration_ready(p) ? "Ready for explicit export" : "Resolve migration issues before session export") + " · " + std::to_string(xc_legacy_migration_tab_count(p)) + " tabs · " + std::to_string(xc_legacy_migration_source_count(p)) + " source files";
		for (size_t i = 0; i < xc_legacy_migration_setting_count(p); ++i) {
			xc_legacy_migration_setting row{};
			if (xc_legacy_migration_setting_get(p, i, &row) == XC_OK)
				migration_review_ += "\n" + copy(row.section) + " / " + copy(row.key) + " = " + copy(row.value) + (row.mapped ? " [mapped]" : " [retained]");
		}
		for (size_t i = 0; i < xc_legacy_migration_issue_count(p); ++i) {
			xc_legacy_migration_issue row{};
			if (xc_legacy_migration_issue_get(p, i, &row) == XC_OK)
				migration_review_ += "\nTab " + std::to_string(row.tab) + " · " + copy(row.module) + " · " + copy(row.reason);
		}
		break;
	}
	case Operation::migration_write: {
		std::unique_ptr<xc_legacy_migration, decltype(&xc_legacy_migration_release)> p(xc_request_take_legacy_migration(done.get()), xc_legacy_migration_release);
		notice_ = "Migration output written to the explicit new destination. Original sources were retained.";
		break;
	}
	case Operation::assets:
	case Operation::assets_copy: {
		auto *p = xc_request_take_legacy_assets(done.get());
		if (!p) {
			error_ = "Missing assets review.";
			break;
		}
		xc_legacy_assets_release(assets_);
		assets_ = p;
		assets_review_ = copy(xc_legacy_assets_source(p)) + " · " + std::to_string(xc_legacy_assets_bytes(p)) + " bytes";
		for (size_t i = 0; i < xc_legacy_assets_count(p); ++i) {
			xc_legacy_asset row{};
			if (xc_legacy_assets_get(p, i, &row) == XC_OK)
				assets_review_ += "\n" + copy(row.path) + (row.directory ? "/" : " · " + std::to_string(row.bytes) + " bytes");
		}
		if (operation_ == Operation::assets_copy)
			notice_ = "Reviewed assets copied to the new destination.";
		break;
	}
	case Operation::legacy_sources: {
		auto *p = xc_request_take_legacy_repository_preview(done.get());
		if (!p) {
			error_ = "Missing legacy source review.";
			break;
		}
		xc_legacy_repository_preview_release(legacy_);
		legacy_ = p;
		legacy_sources_.clear();
		const char *states[] = {"HTTPS needs review", "Insecure transport", "Local directory", "Credentials rejected", "Unsupported type", "Malformed"};
		for (size_t i = 0; i < xc_legacy_repository_preview_count(p); ++i) {
			xc_legacy_repository_candidate row{};
			if (xc_legacy_repository_preview_get(p, i, &row) == XC_OK)
				legacy_sources_.push_back("Line " + std::to_string(row.line) + " · " + copy(row.name) + " · " + copy(row.host) + " · " + (row.disposition >= 0 && row.disposition < 6 ? states[row.disposition] : "Unsupported"));
		}
		break;
	}

	case Operation::catalog: {
		std::unique_ptr<xc_library_catalog, decltype(&xc_library_catalog_release)> p(xc_request_take_library_catalog(done.get()), xc_library_catalog_release);
		if (p)
			copy_catalog(p.get());
		else
			error_ = "Missing catalog.";
		break;
	}
	case Operation::prepare: {
		std::unique_ptr<xc_local_install_preview, decltype(&xc_local_install_preview_release)> p(xc_request_take_local_install_preview(done.get()), xc_local_install_preview_release);
		xc_local_install_info row{};
		if (xc_local_install_preview_get(p.get(), &row) == XC_OK)
			install_ = {copy(row.module), copy(row.source_revision), copy(row.destination_revision), copy(row.source_version), copy(row.installed_version), row.decision};
		else
			error_ = "Missing install review.";
		break;
	}
	case Operation::install: {
		std::unique_ptr<xc_local_install_result, decltype(&xc_local_install_result_release)> p(xc_request_take_local_install_result(done.get()), xc_local_install_result_release);
		xc_local_install_done row{};
		if (xc_local_install_result_get(p.get(), &row) == XC_OK) {
			notice_ = "Installed " + copy(row.module) + " " + copy(row.version);
			++changed_;
			install_ = {};
		} else
			error_ = "Missing install result.";
		break;
	}
	case Operation::maintenance: {
		std::unique_ptr<xc_module_maintenance_preview, decltype(&xc_module_maintenance_preview_release)> p(xc_request_take_module_maintenance_preview(done.get()), xc_module_maintenance_preview_release);
		xc_module_maintenance_info row{};
		if (xc_module_maintenance_preview_get(p.get(), &row) == XC_OK)
			maintenance_ = {copy(row.module), copy(row.revision), row.cipher_declared != 0, row.indexed != 0};
		else
			error_ = "Missing maintenance review.";
		break;
	}
	case Operation::maintain: {
		std::unique_ptr<xc_module_maintenance_result, decltype(&xc_module_maintenance_result_release)> p(xc_request_take_module_maintenance_result(done.get()), xc_module_maintenance_result_release);
		xc_module_maintenance_done row{};
		if (xc_module_maintenance_result_get(p.get(), &row) == XC_OK) {
			notice_ = "Module maintenance completed.";
			maintenance_ = {};
			++changed_;
		} else
			error_ = "Missing maintenance result.";
		break;
	}
	case Operation::repositories:
	case Operation::save_repositories: {
		auto *p = xc_request_take_repository_snapshot(done.get());
		if (!p) {
			error_ = "Missing source profile.";
			break;
		}
		xc_repository_snapshot_release(repositories_);
		repositories_ = p;
		const auto *revision = xc_repository_snapshot_revision(p);
		repository_revision_ = revision ? std::make_optional(std::string(revision)) : std::nullopt;
		copy_repositories();
		notice_ = "Source profile loaded/saved.";
		break;
	}
	case Operation::fetch: {
		auto *p = xc_request_take_remote_catalog(done.get());
		if (!p) {
			error_ = "Missing remote catalog.";
			break;
		}
		xc_remote_catalog_release(remote_);
		remote_ = p;
		xc_remote_install_result_release(remote_result_);
		remote_result_ = nullptr;
		copy_catalog(xc_remote_catalog_library(p));
		if (repositories_ && xc_repository_snapshot_cache(repositories_, p, std::time(nullptr)) == XC_OK) {
			copy_repositories();
			notice_ = "HTTPS catalog fetched. Save sources to retain its offline cache.";
		}
		break;
	}
	case Operation::remote_install: {
		auto *p = xc_request_take_remote_install_result(done.get());
		if (!p) {
			error_ = "Missing remote installation result.";
			break;
		}
		xc_remote_install_result_release(remote_result_);
		remote_result_ = p;
		copy_catalog(xc_remote_catalog_library(remote_catalog()));
		if (repositories_)
			xc_repository_snapshot_cache(repositories_, remote_catalog(), std::time(nullptr));
		notice_ = "Remote module installed. Save sources to retain the updated cache.";
		++changed_;
		break;
	}
	case Operation::organization:
	case Operation::organize: {
		std::unique_ptr<xc_library_organization, decltype(&xc_library_organization_release)> p(xc_request_take_library_organization(done.get()), xc_library_organization_release);
		if (!p) {
			error_ = "Missing organization profile.";
			break;
		}
		organization_revision_ = copy(xc_library_organization_revision(p.get()));
		groups_.clear();
		for (size_t i = 0; i < xc_library_organization_count(p.get()); ++i)
			groups_.push_back({copy(xc_library_organization_provider(p.get(), i)), copy(xc_library_organization_item(p.get(), i)), copy(xc_library_organization_group(p.get(), i)), xc_library_organization_favorite(p.get(), i) != 0, xc_library_organization_hidden(p.get(), i) != 0});
		break;
	}
	}
}
} // namespace xiphos::linux_preview
