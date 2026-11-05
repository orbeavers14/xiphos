
	// old_key is uppercase
	std::string click_key;
	try {
		click_key = xiphos::core::dictionary_click_key(key);
	} catch (const std::invalid_argument &) {
		XI_message(("Dictionary click key is not valid UTF-8"));
		return;
	}
	key = click_key.c_str();
