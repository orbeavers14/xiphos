// SPDX-License-Identifier: GPL-2.0-or-later
#include "book_tree_source.hpp"
#include <swmodule.h>
#include <treekeyidx.h>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
namespace xiphos::core::detail {
namespace {
void valid_offset(std::uint64_t offset) {
  // TreeKeyIdx's on-disk addresses are signed 32-bit index positions, including
  // on LP64 hosts where its public unsigned long argument is wider.
  if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) || offset % 4)
    throw std::invalid_argument("Invalid general-book tree offset");
}
void position(sword::TreeKeyIdx &key, std::uint64_t offset) {
  valid_offset(offset);
  key.popError();
  key.setOffset(static_cast<unsigned long>(offset));
  // Invalid SDK offsets may retain the previous title/path. Check before copying.
  if (key.popError() || key.getOffset() != offset)
    throw std::out_of_range("General-book node does not exist");
}
std::unique_ptr<sword::TreeKeyIdx> source_key(sword::SWModule &module) {
  if (std::string(module.getType()) != "Generic Books")
    throw std::invalid_argument("Module is not a general book");
  std::unique_ptr<sword::SWKey> owned(module.createKey());
  auto *tree = dynamic_cast<sword::TreeKeyIdx *>(owned.get());
  if (!tree) throw std::invalid_argument("General book has no supported indexed tree");
  owned.release();
  return std::unique_ptr<sword::TreeKeyIdx>(tree);
}
BookTreeNode node(sword::TreeKeyIdx &key, const std::string &module) {
  const auto offset = key.getOffset();
  position(key, offset);
  // Bound ancestor traversal before the SDK assembles its full path. This is
  // a query limit, not a validator for every possible corrupt SDK file.
  sword::TreeKeyIdx ancestor(key);
  std::set<unsigned long> visited;
  for (;;) {
    if (visited.size() >= 256 || !visited.insert(ancestor.getOffset()).second)
      throw std::runtime_error("General-book ancestry is cyclic or too deep");
    if (!ancestor.getOffset()) break;
    if (!ancestor.parent() || ancestor.popError())
      throw std::runtime_error("General-book ancestry is unavailable");
    valid_offset(ancestor.getOffset());
  }
  BookTreeNode result;
  result.offset = offset;
  result.title = offset ? key.getLocalName() : module;
  result.path = key.getText();
  result.has_children = key.hasChildren();
  if (key.popError()) throw std::runtime_error("Cannot copy general-book node");
  return result;
}
BookTree snapshot(sword::TreeKeyIdx &key, const std::string &module) {
  BookTree result;
  result.module = module;
  result.node = node(key, module);
  const auto relative = [&](bool (sword::TreeKeyIdx::*step)()) -> std::optional<BookTreeNode> {
    sword::TreeKeyIdx target(key);
    target.popError();
    if (!(target.*step)()) return std::nullopt;
    if (target.popError()) throw std::runtime_error("Cannot traverse general-book tree");
    if (target.getOffset() == result.node.offset)
      throw std::runtime_error("General-book relationship points to itself");
    return node(target, module);
  };
  result.parent = relative(&sword::TreeKeyIdx::parent);
  result.first_child = relative(&sword::TreeKeyIdx::firstChild);
  result.previous_sibling = relative(&sword::TreeKeyIdx::previousSibling);
  result.next_sibling = relative(&sword::TreeKeyIdx::nextSibling);
  return result;
}
}
BookTree capture_book_tree(sword::SWModule &module, std::uint64_t offset) {
  auto key = source_key(module);
  position(*key, offset);
  return snapshot(*key, module.getName());
}
BookTree capture_book_tree_path(sword::SWModule &module, const std::string &path) {
  // Canonical SDK paths begin with '/'; empty selects the actual tree root.
  // Numeric titles therefore cannot be confused with numeric legacy offsets.
  if (path.size() > 4096 || path.find('\0') != std::string::npos || (!path.empty() && path.front() != '/'))
    throw std::invalid_argument("Expected a canonical general-book path");
  auto key = source_key(module);
  key->popError();
  key->setText(path.c_str());
  const std::string actual = key->getText(); // force the SDK's pending path snap
  if (key->popError() || actual != path)
    throw std::out_of_range("General-book path does not exist or is not canonical");
  return snapshot(*key, module.getName());
}
}
