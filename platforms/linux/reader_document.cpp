// SPDX-License-Identifier: GPL-2.0-or-later
#include "reader_document.hpp"
namespace xiphos::linux_preview {
std::string document(const std::string &body, const std::string &css, bool inline_images) {
  // No module scripts, external resources, frames or network requests. Native
  // policy routes user passage clicks through C++; no browser navigation engine.
  return "<!doctype html><html><head><meta charset='utf-8'><meta http-equiv='Content-Security-Policy' content=\"default-src 'none'; style-src 'unsafe-inline'; img-src " + std::string(inline_images ? "data:" : "'none'") + "; script-src 'none'; frame-src 'none'; connect-src 'none'\"><style>body{font:18px serif;line-height:1.55;margin:20px;color:#202020;background:#fff}:root{color-scheme:light dark}span.wordsOfJesus{color:red}.xc-reference{margin-bottom:20px}.xc-reference header{font-weight:bold}.xc-verse,.xc-commentary-section{scroll-margin-top:12px}.xc-current a{color:inherit}.xc-verse sup{font-size:.7em;padding-right:4px}a{color:#174ea6}.xc-verse{padding:.15em 0}.xc-current{background:#fff0b3}.xc-commentary td{padding:4px;vertical-align:top}@media(prefers-color-scheme:dark){body{background:#202020;color:#eee}a{color:#9ecbff}.xc-current{background:#4d421e}}" + css + "</style></head><body>" + body + "</body></html>";
}
}
