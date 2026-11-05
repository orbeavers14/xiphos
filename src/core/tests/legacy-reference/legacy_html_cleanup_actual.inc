void
ClearImages(gchar *text)
{
    const auto result = xiphos::core::blank_images(text);
    memcpy(text, result.html.c_str(), result.html.size());
    if (result.malformed_offset != std::string::npos)
        XI_message(("ClearImages: no img end: %s\n", text + result.malformed_offset));
}

#define CLEAR_FONT_NAME "<font face=\""
#define CLEAR_FONT_NAME_LENGTH 12

void
ClearFontFaces(gchar *text)
{
    const auto result = xiphos::core::blank_font_faces(text);
    memcpy(text, result.html.c_str(), result.html.size());
    if (result.malformed_offset != std::string::npos)
        XI_message(("ClearFontFaces: no closing double quote: %s\n", text + result.malformed_offset));
}

