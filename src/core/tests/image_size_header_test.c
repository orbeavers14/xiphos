/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "xiphos/core/image_size.h"
int image_size_c_header_test(void) {
  xc_image_size bounds = {0, 0}, fitted = {0, 0};
  return xc_image_viewport_size(1000, 800, 2, &bounds) || bounds.width != 402 || bounds.height != 777 ||
    xc_fit_image_size(1600, 1200, bounds.width, bounds.height, &fitted) || fitted.width != 402 || fitted.height != 301;
}
