/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XIPHOS_CORE_IMAGE_SIZE_H
#define XIPHOS_CORE_IMAGE_SIZE_H
#ifdef __cplusplus
extern "C" {
#endif
/* Pure legacy geometry: host owns viewport measurement and image decoding.
 * No GTK, SDK, file access or mutable process state. Inputs are pixel counts.
 * Both calls clear a non-NULL output on invalid input; return 0 on success,
 * 1 on invalid/unrepresentable geometry. Legacy truncation can produce a zero
 * output dimension for extreme aspect ratios; no upscaling or minimum is added.
 */
typedef struct { int width, height; } xc_image_size;
int xc_image_viewport_size(int width, int height, int columns, xc_image_size *out);
int xc_fit_image_size(int image_width, int image_height, int bound_width, int bound_height, xc_image_size *out);
#ifdef __cplusplus
}
#endif
#endif
