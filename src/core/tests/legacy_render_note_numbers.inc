/*
 * Xiphos Bible Study Tool
 * display.cc -
 *
 * Copyright (C) 2000-2026 Xiphos Developer Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

// Complete frozen set_render_numbers, fork master c39b024; unchanged from baseline c9976a8.
void
set_render_numbers(SWModule &imodule, GLOBAL_OPS *ops)
{
	// if we have not yet determined options, don't bother.
	if (!ops)
		return;

	for (FilterList::const_iterator it =
		 imodule.getRenderFilters().begin();
	     it != imodule.getRenderFilters().end();
	     ++it) {
		OSISXHTML *f1 = dynamic_cast<OSISXHTML *>(*it);
		if (f1)
			f1->setRenderNoteNumbers((ops->xrefnotenumbers != 0));
		ThMLXHTML *f2 = dynamic_cast<ThMLXHTML *>(*it);
		if (f2)
			f2->setRenderNoteNumbers((ops->xrefnotenumbers != 0));
		GBFXHTML *f3 = dynamic_cast<GBFXHTML *>(*it);
		if (f3)
			f3->setRenderNoteNumbers((ops->xrefnotenumbers != 0));
		TEIXHTML *f4 = dynamic_cast<TEIXHTML *>(*it);
		if (f4)
			f4->setRenderNoteNumbers((ops->xrefnotenumbers != 0));
	}
}
