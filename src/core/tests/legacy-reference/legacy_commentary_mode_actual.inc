bool actual_commentary_mode(CurrentBackend *backend, const char *mod_name, sword::SWConfig &module_options) {
struct { bool commentary_by_chapter; } value; auto *ops = &value;
ops->commentary_by_chapter =
	    ((backend->module_type(mod_name) == COMMENTARY_TYPE) &&
	     (module_options[mod_name]["Commentary by Chapter"] != "Off"));
return ops->commentary_by_chapter;
}
