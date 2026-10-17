// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/book_resource.hpp"
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <memory>
#include <stdexcept>
namespace xiphos::core {
namespace {
std::string base64(const std::string &bytes) {
  const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string result;
  result.reserve(((bytes.size() + 2) / 3) * 4);
  for (std::size_t i = 0; i < bytes.size(); i += 3) {
    const auto a = static_cast<unsigned char>(bytes[i]);
    const auto b = i+1 < bytes.size() ? static_cast<unsigned char>(bytes[i+1]) : 0;
    const auto c = i+2 < bytes.size() ? static_cast<unsigned char>(bytes[i+2]) : 0;
    result += alphabet[a >> 2]; result += alphabet[((a & 3) << 4) | (b >> 4)];
    result += i+1 < bytes.size() ? alphabet[((b & 15) << 2) | (c >> 6)] : '=';
    result += i+2 < bytes.size() ? alphabet[c & 63] : '=';
  }
  return result;
}
xmlNode *body(xmlNode *node) {
  for (; node; node = node->next) {
    if (node->type == XML_ELEMENT_NODE && xmlStrcasecmp(node->name, BAD_CAST "body") == 0) return node;
    if (auto *found = body(node->children)) return found;
  }
  return nullptr;
}
}
BookInlineImages prepare_book_inline_images(const std::string &html,
    const std::function<BookImageResource(const std::string &, std::size_t)> &load,
    const BookInlineImageLimits &limits) {
  if (!load || !limits.max_images || limits.max_images > 64 || !limits.max_image_bytes ||
      limits.max_image_bytes > maximum_book_image_bytes || !limits.max_html_bytes ||
      limits.max_html_bytes > 32*1024*1024 || html.size() > 8*1024*1024 ||
      html.find('\0') != std::string::npos)
    throw std::invalid_argument("Unsupported inline book image limits or HTML");
  if (html.size() > limits.max_html_bytes) throw std::invalid_argument("Book HTML exceeds the inline output limit");
  BookInlineImages result{html, {}, 0};
  if (html.empty()) return result;
  // HTML parsing does not load a DTD; NONET additionally forbids parser network
  // access. Browser subresource policy remains the native host's responsibility.
  std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> document(htmlReadMemory(html.data(),
      static_cast<int>(html.size()), nullptr, "UTF-8",
      HTML_PARSE_NONET | HTML_PARSE_NODEFDTD | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING), xmlFreeDoc);
  if (!document) { result.issues.push_back({{}, "Unable to parse book image markup"}); return result; }
  auto *content = body(xmlDocGetRootElement(document.get()));
  if (!content) return result;
  std::size_t visited = 0, encoded_bytes = 0;
  std::vector<xmlNode *> pending{content};
  while (!pending.empty()) {
    auto *node = pending.back(); pending.pop_back();
    if (node->next) pending.push_back(node->next);
    if (node->children) pending.push_back(node->children);
    if (node->type != XML_ELEMENT_NODE || xmlStrcasecmp(node->name, BAD_CAST "img") != 0) continue;
    if (visited++ == limits.max_images) {
      result.issues.push_back({{}, "Inline book image count exceeds the limit"}); break;
    }
    std::unique_ptr<xmlChar, decltype(xmlFree)> source(xmlGetProp(node, BAD_CAST "src"), xmlFree);
    if (!source) { result.issues.push_back({{}, "Book image has no source"}); continue; }
    const std::string uri(reinterpret_cast<const char *>(source.get()));
    if (uri.size() > 32768) { result.issues.push_back({{}, "Book image URL exceeds the limit"}); continue; }
    try {
      auto image = load(uri, limits.max_image_bytes);
      if (image.data.size() > limits.max_image_bytes) throw std::invalid_argument("Book image exceeds the byte limit");
      // Only library-provided MIME hints can enter the data URL. This does not
      // validate bytes as an image; the platform decoder still determines that.
      const auto &mime = image.target.command.media_type;
      if (mime != "image/png" && mime != "image/jpeg" && mime != "image/gif" && mime != "image/bmp" &&
          mime != "image/svg+xml" && mime != "image/tiff" && mime != "image/webp")
        throw std::invalid_argument("Unsupported inline book image media type");
      const auto size = ((image.data.size()+2)/3)*4 + mime.size() + 13;
      if (size > limits.max_html_bytes - html.size() - encoded_bytes)
        throw std::invalid_argument("Inline book images exceed the output limit");
      const auto data_url = "data:" + mime + ";base64," + base64(image.data);
      if (!xmlSetProp(node, BAD_CAST "src", BAD_CAST data_url.c_str())) throw std::bad_alloc();
      encoded_bytes += size; ++result.embedded;
    } catch (const std::invalid_argument &error) { result.issues.push_back({uri, error.what()}); }
      catch (const std::runtime_error &error) { result.issues.push_back({uri, error.what()}); }
  }
  if (!result.embedded) return result; // Exact original fallback, no normalization.
  std::unique_ptr<xmlBuffer, decltype(&xmlBufferFree)> output(xmlBufferCreate(), xmlBufferFree);
  if (!output) throw std::bad_alloc();
  std::vector<xmlNode *> fragments;
  // The HTML parser can place an initial style element in a generated head.
  // Retain those nodes as well as body content in the resource-prepared fragment.
  for (auto *node = xmlDocGetRootElement(document.get())->children; node; node = node->next)
    if (node->type == XML_ELEMENT_NODE && xmlStrcasecmp(node->name, BAD_CAST "head") == 0)
      for (auto *child = node->children; child; child = child->next) fragments.push_back(child);
  for (auto *node = content->children; node; node = node->next) fragments.push_back(node);
  for (auto *node : fragments) {
    if (htmlNodeDump(output.get(), document.get(), node) < 0) throw std::runtime_error("Unable to serialize book image markup");
    if (xmlBufferLength(output.get()) > limits.max_html_bytes) {
      result.html = html; result.embedded = 0;
      result.issues.push_back({{}, "Serialized book image markup exceeds the output limit"});
      return result;
    }
  }
  result.html.assign(reinterpret_cast<const char *>(xmlBufferContent(output.get())), xmlBufferLength(output.get()));
  return result;
}
}
