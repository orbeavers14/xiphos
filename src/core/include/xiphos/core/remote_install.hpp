// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "repository.hpp"
#include "local_install.hpp"
#include <atomic>
#include <memory>
#include <stdexcept>
namespace xiphos::core {
namespace detail {struct TransferAccess;}
enum class TransferPhase {queued=0,downloading,extracting,validating,committing,complete,cancelled,failed};
struct TransferProgress {TransferPhase phase;std::uint64_t downloaded,download_total,expanded,entries;};
// One control per job, atomics only. Cancel and commit race on one gate: a
// cancellation that wins prevents installation; after commit begins it loses.
class TransferControl {
 friend struct detail::TransferAccess;
 std::atomic<unsigned> gate_{0};
 std::atomic<TransferPhase> phase_{TransferPhase::queued};
 std::atomic<std::uint64_t> downloaded_{0},total_{0},expanded_{0},entries_{0};
public:
 bool request_cancel() noexcept {unsigned expected=0;return gate_.compare_exchange_strong(expected,1);}
 TransferProgress progress() const noexcept{return {phase_.load(),downloaded_.load(),total_.load(),expanded_.load(),entries_.load()};}
};
class TransferCancelled:public std::runtime_error {public:TransferCancelled():std::runtime_error("Transfer cancelled before installation commit") {}};
struct TransferOptions {
 std::string workspace,ca_bundle;
 std::uint64_t maximum_download_bytes=128*1024*1024,maximum_expanded_bytes=512*1024*1024;
 std::uint32_t maximum_entries=16384,timeout_seconds=90;
 bool remote_access_acknowledged=false;
};
struct RemoteCatalog {
 HttpsRepository repository;
 LibraryCatalog catalog;
 std::string revision,destination_revision,archive_sha256;
};
struct RemoteInstallResult {RemoteCatalog catalog;LocalInstallResult installed;std::string archive_sha256;std::uint64_t downloaded=0;};
bool transfer_options_valid(const TransferOptions &options) noexcept;
}
