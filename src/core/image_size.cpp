// SPDX-License-Identifier: GPL-2.0-or-later
#include "xiphos/core/image_size.h"
#include <limits>
namespace {
bool pixel(float value, int &out) {
  // Compare at wider precision: converting INT_MAX to float rounds upward.
  if (!(value >= 0) || static_cast<long double>(value) > std::numeric_limits<int>::max()) return false;
  out = static_cast<int>(value); return true;
}
}
extern "C" int xc_image_viewport_size(int width, int height, int columns, xc_image_size *out) {
  if (!out) return 1;
  *out = {};
  if (width <= 0 || height <= 0 || columns <= 0) return 1;
  if (columns != 1) {
    width /= columns;
    width = static_cast<int>(static_cast<float>(width) * 0.85);
  }
  if (width > 200 || height > 200) { width -= 23; height -= 23; }
  else { width = (width * 93) / 100; height = (height * 93) / 100; }
  if (width <= 0 || height <= 0) return 1;
  *out = {width, height}; return 0;
}
extern "C" int xc_fit_image_size(int image_width, int image_height, int width, int height, xc_image_size *out) {
  if (!out) return 1;
  *out = {};
  if (image_width <= 0 || image_height <= 0 || width <= 0 || height <= 0) return 1;
  // Preserve the legacy sequence and single-precision truncation: first width,
  // then height. In particular an already-small image is never enlarged.
  if (image_width > width) {
    const float proportion = static_cast<float>(width) / static_cast<float>(image_width);
    image_width = width;
    if (!pixel(static_cast<float>(image_height) * proportion, image_height)) return 1;
  }
  if (image_height > height) {
    const float proportion = static_cast<float>(height) / static_cast<float>(image_height);
    image_height = height;
    if (!pixel(static_cast<float>(image_width) * proportion, image_width)) return 1;
  }
  *out = {image_width, image_height}; return 0;
}
