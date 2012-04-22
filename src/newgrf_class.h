/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_class.h Header file for classes to be used by e.g. NewGRF stations and airports */

#ifndef NEWGRF_CLASS_H
#define NEWGRF_CLASS_H

#include "strings_type.h"

/**
 * Struct containing information relating to NewGRF classes for stations and airports.
 */
template <typename Tspec, typename Tid, Tid Tmax>
struct NewGRFClass {
private:
	uint count;       ///< Number of specs in this class.
	Tspec **spec;     ///< Array of specifications.

	/**
	 * The actual classes.
	 * @note We store pointers to membes of this array in various places outside this class (e.g. to 'name' for GRF string resolving).
	 *       Thus this must be a static array, and cannot be a self-resizing SmallVector or similar.
	 */
	static NewGRFClass<Tspec, Tid, Tmax> classes[Tmax];

	/** Initialise the defaults. */
	static void InsertDefaults();

public:
	uint32 global_id; ///< Global ID for class, e.g. 'DFLT', 'WAYP', etc.
	StringID name;    ///< Name of this class.

	static void Reset();
	static Tid Allocate(uint32 global_id);
	static void Assign(Tspec *spec);
	static NewGRFClass *Get(Tid cls_id);

	static uint GetCount();
	static uint GetCount(Tid cls_id);
	static const Tspec *Get(Tid cls_id, uint index);
	static const Tspec *GetByGrf(uint32 grfid, byte local_id, int *index);
};

#endif /* NEWGRF_CLASS_H */
