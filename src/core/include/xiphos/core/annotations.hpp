// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
namespace xiphos::core {
inline constexpr std::size_t maximum_annotation_records=10000;
inline constexpr std::size_t maximum_annotation_label_bytes=4096;
inline constexpr std::size_t maximum_annotation_note_bytes=1024*1024;
inline constexpr std::size_t maximum_annotation_snapshot_bytes=16*1024*1024;
struct AnnotationAddress {
  std::string module,book;
  int chapter=0,verse=0,packed=0; // Retained1000*chapter+verse cache key.
};
struct AnnotationRecord {
  std::string element;
  std::optional<std::string> label,note;
  std::optional<AnnotationAddress> address;
  bool legacy_visible=false; // Cursor prefix before first child without a label.
};
struct AnnotationProfile {
  std::vector<AnnotationRecord> records; // All element children of first section.
  std::optional<std::size_t> legacy_stop_child;
  std::size_t shadowed_sections=0,snapshot_bytes=0;
};
struct VerseAnnotation {
  int chapter=0,verse=0,packed=0;
  std::size_t record=0;
  std::string html; // Plain note escaped with exact retained six replacements.
};
struct AnnotationCache {
  std::string module,book;
  std::vector<VerseAnnotation> entries; // Packed-key order, last matching row wins.
  bool incomplete=false;
  std::optional<std::size_t> malformed_record;
  std::size_t snapshot_bytes=0;
};
// Rightmost two dots + first space, bounded atoi-prefix semantics. Missing shape
// or unsafe integer/packed overflow yields no address; no SDK reference guessing.
std::optional<AnnotationAddress> decode_annotation_label(const std::string &label);
std::string annotation_note_html(const std::string &note);
// Explicit Xiphos/osisrefmarkedverses metadata; preserve missing/empty and raw
// unknown element labels. No home lookup, conversion, rendering or writes.
AnnotationProfile parse_annotations(const std::string &xml);
AnnotationProfile read_annotations(const std::string &path);
enum class AnnotationAction { save,remove };
struct AnnotationEdit {
  AnnotationAction action=AnnotationAction::save;
  std::string label,note;
};
void validate_annotation_edit(const AnnotationEdit &edit);
// Save replaces the first markedverse with the exact label or appends one. Remove
// deletes only its first matching item. Unknown fields/duplicates/sections remain.
// New saves require safe canonical positive chapter/verse labels (verse<1000).
// Remove also accepts malformed raw labels so imported records can be repaired.
std::string serialize_annotation_edit(const std::string &xml,const AnnotationEdit &edit);
AnnotationProfile edit_annotations(const std::string &path,const AnnotationEdit &edit);
// Characterized legacy cursor behavior, including unlabeled/whitespace barriers
// and malformed-label prefix abort. Generic module matches all; ASCII-insensitive
// module/book comparison. No installed module or active tab required.
AnnotationCache legacy_annotation_cache(const AnnotationProfile &profile,const std::string &module,const std::string &book);
}
