/*
 * Xiphos Bible Study Tool
 * sword.cc - glue
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

// Frozen from src/main/sword.cc at a112980. Defined valid-input behavior only.
static std::map<std::string, std::string> abbrev_name2abbrev, abbrev_abbrev2name;
typedef std::map<std::string, std::string>::iterator abbrev_iter;
void main_add_abbreviation(const char *name, const char *abbreviation)
{
	// let's not be stupid about abbreviations chosen, ok?
	if (!strchr(abbreviation, '(')) {
		abbrev_name2abbrev[name] = abbreviation;
		abbrev_abbrev2name[abbreviation] = name;
	}
}

void main_clear_abbreviations(void)
{
	abbrev_name2abbrev.clear();
	abbrev_abbrev2name.clear();
}

const char *main_name_to_abbrev(const char *name)
{
	if (name == NULL)
		return NULL;
	abbrev_iter it = abbrev_name2abbrev.find(name);
	if (it != abbrev_name2abbrev.end()) {
		return it->second.c_str();
	}
	return NULL;
}

const char *main_abbrev_to_name(const char *abbreviation)
{
	if (abbreviation == NULL)
		return NULL;
	abbrev_iter it = abbrev_abbrev2name.find(abbreviation);
	if (it != abbrev_abbrev2name.end()) {
		return it->second.c_str();
	}
	return NULL;
}

