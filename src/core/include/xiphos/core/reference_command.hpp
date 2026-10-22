// SPDX-License-Identifier: GPL-2.0-or-later
// Extracted from main/url.cc, Copyright (C) 2004-2026 Xiphos Developer Team.
#ifndef XIPHOS_CORE_REFERENCE_COMMAND_HPP
#define XIPHOS_CORE_REFERENCE_COMMAND_HPP
namespace xiphos { namespace core {
// Retained show_ref dispatch, usable by the C++11 GTK adapter. Inputs are
// borrowed only for this call. Membership must be exact installed-ID membership,
// not abbreviation resolution. Hover performs neither lookup nor presentation.
// The host owns list parsing/rendering and must copy values it retains.
template<class Installed, class Display>
int dispatch_reference_command(const char *requested_module, const char *main_module,
                               const char *context, const char *expression, bool clicked,
                               Installed installed, Display display) {
  if (!clicked) return 1;
  const char *module = requested_module;
  if (!installed(module)) module = main_module;
  display(context, module, expression);
  return 1;
}
}}
#endif
