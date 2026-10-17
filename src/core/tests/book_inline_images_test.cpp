// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/book_resource.hpp"
#include <iostream>
#include <stdexcept>
using namespace xiphos::core;
void check(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
int main() { try {
  std::size_t calls = 0;
  const auto load = [&](const std::string &source, std::size_t limit) -> BookImageResource {
    ++calls;
    check(limit <= maximum_book_image_bytes,"Caller passes bounded image limit");
    if(source=="missing.png") throw std::invalid_argument("Original missing resource");
    check(source=="a+b.png?x=1&y=2" || source=="one.png" || source=="two.png" || source=="three.png", "Parser decodes HTML entities without parsing script/comment contents");
    std::string bytes = source=="one.png" ? "f" : source=="two.png" ? "fo" : "foo";
    return {{{"Original",{},{},{},{}},{source,{},"image/png"}},bytes};
  };
  const std::string original="<!-- <img src='trap.png'> --><script>const text=\"<img src='trap.png'>\";</script><p>Original café <a href='passagestudy.jsp?action=showNote&amp;value=1'>note</a><IMG ALT='Original plus' SRC='a+b.png?x=1&amp;y=2' width='8'><img src=missing.png></p>";
  auto result=prepare_book_inline_images(original,load);
  check(calls==2 && result.embedded==1 && result.issues.size()==1 && result.issues[0].source=="missing.png","Only actual img elements load, missing image is reported independently");
  check(result.html.find("data:image/png;base64,Zm9v")!=std::string::npos && result.html.find("width=\"8\"")!=std::string::npos && result.html.find("Original plus")!=std::string::npos && result.html.find("showNote&amp;value=1")!=std::string::npos,"Resource serialization preserves image dimensions, alt and semantic links");
  auto style=prepare_book_inline_images("<style>.original{color:blue}</style><p class=original><img src=one.png></p>",load);
  check(style.embedded==1 && style.html.find(".original{color:blue}")!=std::string::npos && style.html.find("class=\"original\"")!=std::string::npos,"Recovered head styles remain in the prepared fragment");
  auto padding=prepare_book_inline_images("<img src=one.png><img src=two.png><img src=three.png>",load);
  check(padding.embedded==3 && padding.html.find("base64,Zg==")!=std::string::npos && padding.html.find("base64,Zm8=")!=std::string::npos && padding.html.find("base64,Zm9v")!=std::string::npos,"RFC4648 base64 has correct one/two/three-byte padding");
  const std::string missing="<p>Original unmodified <img src='missing.png'></p>";
  check(prepare_book_inline_images(missing,load).html==missing,"No successful resources retains exact source HTML");
  BookInlineImageLimits count;count.max_images=1;
  auto limited=prepare_book_inline_images("<img src=one.png><img src=two.png>",load,count);
  check(limited.embedded==1 && limited.issues.size()==1,"Image count bound reports remainder without loading it");
  BookInlineImageLimits bytes;bytes.max_html_bytes=80;
  auto overflow=prepare_book_inline_images("<p>Original</p><img src=one.png><img src=two.png>",load,bytes);
  check(overflow.html.size()<=bytes.max_html_bytes && !overflow.issues.empty(),"Encoded and serialized output bounds do not truncate HTML");
  auto empty=prepare_book_inline_images("",load);check(empty.html.empty() && !empty.embedded && empty.issues.empty(),"Empty document stays empty");
  auto no_source=prepare_book_inline_images("<img alt=Original>",load);check(no_source.issues.size()==1&&!no_source.embedded,"Missing source is explicit");
  auto plain=prepare_book_inline_images("<p>Original &amp; untouched</p>",load);check(plain.html=="<p>Original &amp; untouched</p>","No images preserves presentation bytes");
  try { auto bad=count;bad.max_images=0;(void)prepare_book_inline_images(original,load,bad);throw std::runtime_error("Expected zero limit rejection"); } catch(const std::invalid_argument &) {}
  try { (void)prepare_book_inline_images(std::string("bad\0html",8),load);throw std::runtime_error("Expected NUL rejection"); } catch(const std::invalid_argument &) {}
  std::cout<<"Owned inline image HTML, entity handling, base64 and bounded fallback passed\n";return 0;
 } catch(const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; }
}
