// SPDX-License-Identifier: GPL-2.0-or-later
#include "book_tree_source.hpp"
#include "sdk_render.hpp"
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
  if (!indexed_book(module))
    throw std::invalid_argument("Module has no supported general-book indexed key");
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
bool indexed_book(const sword::SWModule &module) {
  return std::string(module.getType()) == "Generic Books" &&
    dynamic_cast<sword::TreeKeyIdx *>(module.getKey());
}
BookTree capture_book_tree(sword::SWModule &module, std::uint64_t offset) {
  auto key = source_key(module);
  position(*key, offset);
  return snapshot(*key, module.getName());
}
namespace {
BookTree capture_path(sword::SWModule &module, const std::string &path, bool canonical) {
  // Canonical SDK paths begin with '/'; empty selects the actual tree root.
  // Numeric titles therefore cannot be confused with numeric legacy offsets.
  if (path.size() > 4096 || path.find('\0') != std::string::npos || (canonical && !path.empty() && path.front() != '/'))
    throw std::invalid_argument("Expected a canonical general-book path");
  auto key = source_key(module);
  key->popError();
  key->setText(path.c_str());
  const std::string actual = key->getText(); // force the SDK's pending path snap
  if (key->popError() || (canonical && actual != path))
    throw std::out_of_range("General-book path does not exist or is not canonical");
  return snapshot(*key, module.getName());
}
}
BookTree capture_book_tree_path(sword::SWModule &module, const std::string &path) {
  return capture_path(module, path, true);
}
BookTree capture_book_display_path(sword::SWModule &module, const std::string &path) {
  return capture_path(module, path, false);
}
namespace {
BookEntry capture_entry(sword::SWModule &module, std::uint64_t offset, bool *rendered) {
  BookEntry result;
  result.tree = capture_book_tree(module, offset);
  auto *key = dynamic_cast<sword::TreeKeyIdx *>(module.getKey());
  if (!key) throw std::invalid_argument("General book has no live indexed key");
  // Do not setKey(path): distinct tree addresses (including linked entries)
  // must retain the selected node identity rather than resolving a title again.
  position(*key, offset);
  result.has_entry = module.hasEntry(key);
  if (rendered) { const auto *raw = module.getRawEntry(); *rendered = raw && *raw; }
  if (!rendered || *rendered) result.html = render_sdk_entry(module).c_str();
  if (module.getKey() != key || key->getOffset() != offset ||
      result.tree.node.path != key->getText() || key->getError())
    throw std::runtime_error("General-book render changed the requested source");
  return result;
}
}
BookEntry capture_book_entry(sword::SWModule &module, std::uint64_t offset) {
  return capture_entry(module, offset, nullptr);
}
BookDocument capture_book_document(sword::SWModule &module, std::uint64_t offset, const BookDocumentLimits &limits) {
  const BookDocumentLimits maximum;
  if (!limits.max_depth || limits.max_depth > maximum.max_depth || !limits.max_nodes ||
      limits.max_nodes > maximum.max_nodes || !limits.max_html_bytes || limits.max_html_bytes > maximum.max_html_bytes)
    throw std::invalid_argument("Unsupported general-book document limits");
  BookDocument result;
  result.tree = capture_book_tree(module, offset);
  const auto *configured = module.getConfigEntry("DisplayLevel");
  result.display_level = book_display_level(configured ? std::make_optional(std::string(configured)) : std::nullopt);
  auto *key = dynamic_cast<sword::TreeKeyIdx *>(module.getKey());
  if (!key) throw std::invalid_argument("General book has no live indexed key");
  position(*key, offset);
  // Restore the selected cursor on success and bounded/render failures. Never
  // replace the module's key object; entry-attribute flags are scoped per render.
  struct Cursor {
    sword::SWModule &module; sword::TreeKeyIdx *key; std::uint64_t offset;
    ~Cursor() noexcept {
      try { if (module.getKey() == key) { position(*key, offset); module.getRawEntry(); position(*key, offset); } }
      catch (...) { /* Preserve the original exception; successful exit checks below. */ }
    }
  } restore{module, key, offset};
  struct Pending { std::uint64_t offset; std::uint32_t depth; bool siblings; };
  std::vector<Pending> pending{{offset, 1, false}};
  std::set<std::uint64_t> visited;
  while (!pending.empty()) {
    const auto next = pending.back(); pending.pop_back();
    if (next.depth > limits.max_depth || visited.size() >= limits.max_nodes)
      throw std::runtime_error("General-book document exceeds traversal limits");
    if (!visited.insert(next.offset).second)
      throw std::runtime_error("General-book document traversal repeats a node");
    BookDocumentSection section;
    section.depth = next.depth;
    section.rendered = true;
    section.entry = capture_entry(module, next.offset, result.display_level > 1 ? &section.rendered : nullptr);
    const auto &tree = section.entry.tree;
    if (section.rendered) {
      const std::string separator = result.html.empty() ? "" : "<br/><hr/>";
      if (section.entry.html.size() > limits.max_html_bytes ||
          separator.size() > limits.max_html_bytes - section.entry.html.size() ||
          result.html.size() > limits.max_html_bytes - section.entry.html.size() - separator.size())
        throw std::runtime_error("General-book document exceeds HTML size limit");
      result.html += separator;
      result.html += section.entry.html;
    }
    if (next.siblings && tree.next_sibling)
      pending.push_back({tree.next_sibling->offset, next.depth, true});
    if (next.depth < result.display_level && tree.first_child)
      pending.push_back({tree.first_child->offset, next.depth + 1, true});
    result.sections.push_back(std::move(section));
  }
  position(*key, offset);
  module.getRawEntry();
  if (module.getKey() != key || key->getOffset() != offset || key->popError() ||
      result.tree.node.path != key->getText())
    throw std::runtime_error("General-book document changed the selected source");
  return result;
}
BookChildren capture_book_children(sword::SWModule &module, std::uint64_t parent,
                                   std::optional<std::uint64_t> after, std::uint32_t limit) {
  if (!limit || limit > 256) throw std::invalid_argument("Book child page limit must be 1 through 256");
  auto key = source_key(module);
  position(*key, parent);
  BookChildren result;
  result.module = module.getName();
  result.parent = node(*key, result.module);
  const auto direct_child = [&] {
    sword::TreeKeyIdx owner(*key);
    if (!owner.parent() || owner.popError() || owner.getOffset() != parent)
      throw std::invalid_argument("Book child/cursor belongs to a different parent");
  };
  std::set<unsigned long> visited;
  if (after) {
    position(*key, *after);
    direct_child();
    visited.insert(key->getOffset());
    if (!key->nextSibling()) return result;
  } else if (!key->firstChild()) return result;
  for (;;) {
    if (key->popError()) throw std::runtime_error("Cannot enumerate book children");
    if (!visited.insert(key->getOffset()).second)
      throw std::runtime_error("Book child page contains a sibling cycle");
    auto child = node(*key, result.module);
    direct_child();
    // Validate one lookahead before promising another page. Work/memory remain
    // bounded by limit + 1 nodes, without rescanning all preceding siblings.
    if (result.children.size() == limit) {
      result.next_after = result.children.back().offset;
      break;
    }
    result.children.push_back(std::move(child));
    if (!key->nextSibling()) break;
  }
  return result;
}
BookNote capture_book_note(sword::SWModule &module, std::uint64_t offset,
                           const std::string &identifier, bool render_body) {
  if (identifier.empty() || identifier.find('\0') != std::string::npos)
    throw std::invalid_argument("Book note requires a valid identifier");
  BookNote result;
  result.tree = capture_book_tree(module, offset);
  auto *key = dynamic_cast<sword::TreeKeyIdx *>(module.getKey());
  if (!key) throw std::invalid_argument("General book has no live indexed key");
  position(*key, offset);
  result.note = capture_study_note_at_key(module, identifier, render_body);
  if (module.getKey() != key || key->getOffset() != offset ||
      result.tree.node.path != key->getText() || key->getError() ||
      result.note.reference != result.tree.node.path)
    throw std::runtime_error("Book note changed the requested source");
  return result;
}
}
