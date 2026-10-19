/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XIPHOS_CORE_EXPORT_ITEM_H
#define XIPHOS_CORE_EXPORT_ITEM_H
#ifdef __cplusplus
extern "C" {
#endif
/* Shared legacy item formatting only, no SDK/file/clipboard access. Required
 * metadata must be non-NULL. Scripture may be NULL (absent); empty HTML retains
 * an empty li. html is strict 0/1.
 * HTML metadata is escaped; scripture is already-rendered trusted module HTML.
 * Returns owned UTF-8/byte text, or NULL on invalid input/C++ allocation failure.
 * Release ONLY with xiphos_export_item_free. No execution/output cap is implied. */
char *xiphos_export_item(const char *description,const char *module,const char *key,const char *scripture,int html);
void xiphos_export_item_free(char *text);
#ifdef __cplusplus
}
#endif
#endif
