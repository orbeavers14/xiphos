// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "xiphos/core/c_api.h"
#include <optional>
#include <string>
#include <vector>
namespace xiphos::linux_preview
{
struct LibraryItem {
	std::string provider, id, title, language, type, version, license;
	int access = 0;
	unsigned capabilities = 0;
	bool installed = false;
};
struct LibraryEndpoint {
	std::string id, host, catalog, packages, name;
	unsigned port = 443;
	bool enabled = false, cached = false;
	uint64_t fetched = 0;
};
struct LibraryOrganization {
	std::string provider, item, group;
	bool favorite = false, hidden = false;
};
struct InstallReview {
	std::string module, source, destination, source_version, installed_version;
	int decision = -1;
};
struct MaintenanceReview {
	std::string module, revision;
	bool cipher = false, indexed = false;
};
class LibraryModel
{
      public:
	explicit LibraryModel(xc_worker &worker) : worker_(&worker) {}
	~LibraryModel();
	bool catalog(const std::string &root = "");
	bool prepare_install(const std::string &root, const std::string &module);
	bool install(bool replace);
	bool review_maintenance(const std::string &module);
	bool maintain(int action, const std::string &key = "");
	bool repositories(const std::string &path, bool create = false);
	bool put_repository(const LibraryEndpoint &endpoint);
	bool remove_repository(const std::string &id);
	bool move_repository(const std::string &id, size_t position);
	bool save_repositories();
	bool fetch(const std::string &id, const std::string &workspace, bool acknowledged, const std::string &ca_bundle = "");
	bool use_cached(size_t index);
	bool install_remote(const std::string &module, const std::string &workspace, bool replace, bool acknowledged, const std::string &ca_bundle = "");
	bool prepare_migration(const std::string &settings, const std::string &tabs, const std::string &options, const std::vector<std::pair<std::string, std::string>> &mapping = {});
	bool read_migration(const std::string &path, const std::vector<std::pair<std::string, std::string>> &mapping = {});
	bool write_migration(const std::string &destination, int action);
	bool inspect_assets(const std::string &source);
	bool copy_assets(const std::string &destination);
	bool preview_legacy_sources(const std::string &path);
	bool accept_legacy_source(size_t index, const std::string &id, const std::string &packages);
	const std::string &migration_review() const { return migration_review_; }
	const std::string &assets_review() const { return assets_review_; }
	const std::vector<std::string> &legacy_sources() const { return legacy_sources_; }
	bool organization(const std::string &path, bool create = false);
	bool organize(const LibraryOrganization &entry);
	void tick();
	bool busy() const { return request_ != nullptr; }
	bool cancel();
	const std::string &error() const { return error_; }
	const std::string &notice() const { return notice_; }
	const std::vector<LibraryItem> &items() const { return items_; }
	const std::vector<LibraryEndpoint> &endpoints() const { return endpoints_; }
	const std::vector<LibraryOrganization> &groups() const { return groups_; }
	const InstallReview &install_review() const { return install_; }
	const MaintenanceReview &maintenance() const { return maintenance_; }
	const xc_transfer_progress &progress() const { return progress_; }
	unsigned version() const { return version_; }
	unsigned changed() const { return changed_; }

      private:
	enum class Operation { migration,
			       migration_write,
			       assets,
			       assets_copy,
			       legacy_sources,
			       catalog,
			       prepare,
			       install,
			       maintenance,
			       maintain,
			       repositories,
			       save_repositories,
			       fetch,
			       remote_install,
			       organization,
			       organize } operation_ = Operation::catalog;
	xc_worker *worker_;
	xc_request *request_ = nullptr;
	xc_transfer_control *control_ = nullptr;
	xc_repository_snapshot *repositories_ = nullptr;
	xc_remote_catalog *remote_ = nullptr;
	xc_remote_install_result *remote_result_ = nullptr;
	xc_legacy_migration *migration_ = nullptr;
	xc_legacy_assets *assets_ = nullptr;
	xc_legacy_repository_preview *legacy_ = nullptr;
	std::string migration_review_, assets_review_;
	std::vector<std::string> legacy_sources_;
	std::vector<LibraryItem> items_;
	std::vector<LibraryEndpoint> endpoints_;
	std::vector<LibraryOrganization> groups_;
	InstallReview install_;
	MaintenanceReview maintenance_;
	xc_transfer_progress progress_{};
	std::string error_, notice_, source_root_, repository_path_, organization_path_, remote_source_id_;
	std::optional<std::string> repository_revision_, organization_revision_;
	unsigned version_ = 0, changed_ = 0;
	bool accept(xc_status, xc_request *, Operation);
	void copy_catalog(const xc_library_catalog *catalog);
	void copy_repositories();
	const xc_remote_catalog *remote_catalog() const;
};
} // namespace xiphos::linux_preview
