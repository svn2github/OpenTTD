/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ini_load.cpp Definition of the #IniLoadFile class, related to reading and storing '*.ini' files. */

#include "stdafx.h"
#include "core/alloc_func.hpp"
#include "core/mem_func.hpp"
#include "debug.h"
#include "ini_type.h"
#include "string_func.h"
#include "fileio_func.h"

/**
 * Construct a new in-memory item of an Ini file.
 * @param parent the group we belong to
 * @param name   the name of the item
 * @param len    the length of the name of the item
 */
IniItem::IniItem(IniGroup *parent, const char *name, size_t len) : next(NULL), value(NULL), comment(NULL)
{
	if (len == 0) len = strlen(name);

	this->name = strndup(name, len);
	*parent->last_item = this;
	parent->last_item = &this->next;
}

/** Free everything we loaded. */
IniItem::~IniItem()
{
	free(this->name);
	free(this->value);
	free(this->comment);

	delete this->next;
}

/**
 * Replace the current value with another value.
 * @param value the value to replace with.
 */
void IniItem::SetValue(const char *value)
{
	free(this->value);
	this->value = strdup(value);
}

/**
 * Construct a new in-memory group of an Ini file.
 * @param parent the file we belong to
 * @param name   the name of the group
 * @param len    the length of the name of the group
 */
IniGroup::IniGroup(IniLoadFile *parent, const char *name, size_t len) : next(NULL), type(IGT_VARIABLES), item(NULL), comment(NULL)
{
	if (len == 0) len = strlen(name);

	this->name = strndup(name, len);
	this->last_item = &this->item;
	*parent->last_group = this;
	parent->last_group = &this->next;

	if (parent->list_group_names == NULL) return;

	for (uint i = 0; parent->list_group_names[i] != NULL; i++) {
		if (strcmp(this->name, parent->list_group_names[i]) == 0) {
			this->type = IGT_LIST;
			return;
		}
	}
}

/** Free everything we loaded. */
IniGroup::~IniGroup()
{
	free(this->name);
	free(this->comment);

	delete this->item;
	delete this->next;
}

/**
 * Get the item with the given name, and if it doesn't exist
 * and create is true it creates a new item.
 * @param name   name of the item to find.
 * @param create whether to create an item when not found or not.
 * @return the requested item or NULL if not found.
 */
IniItem *IniGroup::GetItem(const char *name, bool create)
{
	for (IniItem *item = this->item; item != NULL; item = item->next) {
		if (strcmp(item->name, name) == 0) return item;
	}

	if (!create) return NULL;

	/* otherwise make a new one */
	return new IniItem(this, name, strlen(name));
}

/**
 * Clear all items in the group
 */
void IniGroup::Clear()
{
	delete this->item;
	this->item = NULL;
	this->last_item = &this->item;
}

/**
 * Construct a new in-memory Ini file representation.
 * @param list_group_names A NULL terminated list with groups that should be
 *                         loaded as lists instead of variables.
 */
IniLoadFile::IniLoadFile(const char * const *list_group_names) : group(NULL), comment(NULL), list_group_names(list_group_names)
{
	this->last_group = &this->group;
}

/** Free everything we loaded. */
IniLoadFile::~IniLoadFile()
{
	free(this->comment);
	delete this->group;
}

/**
 * Get the group with the given name, and if it doesn't exist
 * create a new group.
 * @param name name of the group to find.
 * @param len  the maximum length of said name.
 * @return the requested group.
 */
IniGroup *IniLoadFile::GetGroup(const char *name, size_t len)
{
	if (len == 0) len = strlen(name);

	/* does it exist already? */
	for (IniGroup *group = this->group; group != NULL; group = group->next) {
		if (!strncmp(group->name, name, len) && group->name[len] == 0) {
			return group;
		}
	}

	/* otherwise make a new one */
	IniGroup *group = new IniGroup(this, name, len);
	group->comment = strdup("\n");
	return group;
}

/**
 * Remove the group with the given name.
 * @param name name of the group to remove.
 */
void IniLoadFile::RemoveGroup(const char *name)
{
	size_t len = strlen(name);
	IniGroup *prev = NULL;
	IniGroup *group;

	/* does it exist already? */
	for (group = this->group; group != NULL; prev = group, group = group->next) {
		if (strncmp(group->name, name, len) == 0) {
			break;
		}
	}

	if (group == NULL) return;

	if (prev != NULL) {
		prev->next = prev->next->next;
		if (this->last_group == &group->next) this->last_group = &prev->next;
	} else {
		this->group = this->group->next;
		if (this->last_group == &group->next) this->last_group = &this->group;
	}

	group->next = NULL;
	delete group;
}

/**
 * Load the Ini file's data from the disk.
 * @param filename the file to load.
 * @pre nothing has been loaded yet.
 */
void IniLoadFile::LoadFromDisk(const char *filename)
{
	assert(this->last_group == &this->group);

	char buffer[1024];
	IniGroup *group = NULL;

	char *comment = NULL;
	uint comment_size = 0;
	uint comment_alloc = 0;

	size_t end;
	/*
	 * Now we are going to open a file that contains no more than simple
	 * plain text. That would raise the question: "why open the file as
	 * if it is a binary file?". That's simple... Microsoft, in all
	 * their greatness and wisdom decided it would be useful if ftell
	 * is aware of '\r\n' and "sees" that as a single character. The
	 * easiest way to test for that situation is by searching for '\n'
	 * and decrease the value every time you encounter a '\n'. This will
	 * thus also make ftell "see" the '\r' when it is not there, so the
	 * result of ftell will be highly unreliable. So to work around this
	 * marvel of wisdom we have to open in as a binary file.
	 */
	FILE *in = FioFOpenFile(filename, "rb", DATA_DIR, &end);
	if (in == NULL) return;

	end += ftell(in);

	/* for each line in the file */
	while ((size_t)ftell(in) < end && fgets(buffer, sizeof(buffer), in)) {
		char c, *s;
		/* trim whitespace from the left side */
		for (s = buffer; *s == ' ' || *s == '\t'; s++) {}

		/* trim whitespace from right side. */
		char *e = s + strlen(s);
		while (e > s && ((c = e[-1]) == '\n' || c == '\r' || c == ' ' || c == '\t')) e--;
		*e = '\0';

		/* skip comments and empty lines */
		if (*s == '#' || *s == ';' || *s == '\0') {
			uint ns = comment_size + (e - s + 1);
			uint a = comment_alloc;
			/* add to comment */
			if (ns > a) {
				a = max(a, 128U);
				do a *= 2; while (a < ns);
				comment = ReallocT(comment, comment_alloc = a);
			}
			uint pos = comment_size;
			comment_size += (e - s + 1);
			comment[pos + e - s] = '\n'; // comment newline
			memcpy(comment + pos, s, e - s); // copy comment contents
			continue;
		}

		/* it's a group? */
		if (s[0] == '[') {
			if (e[-1] != ']') {
				ShowInfoF("ini: invalid group name '%s'", buffer);
			} else {
				e--;
			}
			s++; // skip [
			group = new IniGroup(this, s, e - s);
			if (comment_size != 0) {
				group->comment = strndup(comment, comment_size);
				comment_size = 0;
			}
		} else if (group != NULL) {
			char *t;
			/* find end of keyname */
			if (*s == '\"') {
				s++;
				for (t = s; *t != '\0' && *t != '\"'; t++) {}
				if (*t == '\"') *t = ' ';
			} else {
				for (t = s; *t != '\0' && *t != '=' && *t != '\t' && *t != ' '; t++) {}
			}

			/* it's an item in an existing group */
			IniItem *item = new IniItem(group, s, t - s);
			if (comment_size != 0) {
				item->comment = strndup(comment, comment_size);
				comment_size = 0;
			}

			/* find start of parameter */
			while (*t == '=' || *t == ' ' || *t == '\t') t++;

			bool quoted = (*t == '\"');
			/* remove starting quotation marks */
			if (*t == '\"') t++;
			/* remove ending quotation marks */
			e = t + strlen(t);
			if (e > t && e[-1] == '\"') e--;
			*e = '\0';

			/* If the value was not quoted and empty, it must be NULL */
			item->value = (!quoted && e == t) ? NULL : strndup(t, e - t);
		} else {
			/* it's an orphan item */
			ShowInfoF("ini: '%s' outside of group", buffer);
		}
	}

	if (comment_size > 0) {
		this->comment = strndup(comment, comment_size);
		comment_size = 0;
	}

	free(comment);
	fclose(in);
}

