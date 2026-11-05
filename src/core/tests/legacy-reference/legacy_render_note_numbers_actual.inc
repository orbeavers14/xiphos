void
set_render_numbers(SWModule &imodule, GLOBAL_OPS *ops)
{
    if (!ops) return;
    xiphos::core::set_render_note_numbers(imodule, ops->xrefnotenumbers != 0);
}
