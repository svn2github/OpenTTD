/* $Id$ */

/** @file railtypes.h
 * All the railtype-specific information is stored here.
 */

/** Global Railtype definition
 */
const RailtypeInfo _railtypes[RAILTYPE_END] = {
	/** Railway */
	{ /* Main Sprites */
		{ SPR_RAIL_TRACK_Y, SPR_RAIL_TRACK_N_S, SPR_RAIL_TRACK_BASE, SPR_RAIL_SINGLE_Y, SPR_RAIL_SINGLE_X,
			SPR_RAIL_SINGLE_NORTH, SPR_RAIL_SINGLE_SOUTH, SPR_RAIL_SINGLE_EAST, SPR_RAIL_SINGLE_WEST,
			SPR_CROSSING_OFF_X_RAIL,
			SPR_TUNNEL_ENTRY_REAR_RAIL
		},

		/* GUI sprites */
		{ 0x4E3, 0x4E4, 0x4E5, 0x4E6,
			SPR_OPENTTD_BASE + 0, 0x50E, 0x97E, SPR_OPENTTD_BASE + 25 },

		/* strings */
		{ STR_100A_RAILROAD_CONSTRUCTION },

		/* Offset of snow tiles */
		SPR_RAIL_SNOW_OFFSET,

		/* Compatible railtypes */
		(1 << RAILTYPE_RAIL),

		/* main offset */
		0,
	},

	/** Monorail */
	{ /* Main Sprites */
		{ SPR_MONO_TRACK_Y, SPR_MONO_TRACK_N_S, SPR_MONO_TRACK_BASE, SPR_MONO_SINGLE_Y, SPR_MONO_SINGLE_X,
			SPR_MONO_SINGLE_NORTH, SPR_MONO_SINGLE_SOUTH, SPR_MONO_SINGLE_EAST, SPR_MONO_SINGLE_WEST,
			SPR_CROSSING_OFF_X_MONO,
			SPR_TUNNEL_ENTRY_REAR_MONO
		},

		/* GUI sprites */
		{ 0x4E7, 0x4E8, 0x4E9, 0x4EA,
			SPR_OPENTTD_BASE + 1, SPR_OPENTTD_BASE + 12, 0x97F, SPR_OPENTTD_BASE + 27 },

		/* strings */
		{ STR_100B_MONORAIL_CONSTRUCTION },

		/* Offset of snow tiles */
		SPR_MONO_SNOW_OFFSET,

		/* Compatible Railtypes */
		(1 << RAILTYPE_MONO),

		/* main offset */
		82,
	},

	/** Maglev */
	{ /* Main sprites */
		{ SPR_MGLV_TRACK_Y, SPR_MGLV_TRACK_N_S, SPR_MGLV_TRACK_BASE, SPR_MGLV_SINGLE_Y, SPR_MGLV_SINGLE_X,
			SPR_MGLV_SINGLE_NORTH, SPR_MGLV_SINGLE_SOUTH, SPR_MGLV_SINGLE_EAST, SPR_MGLV_SINGLE_WEST,
			SPR_CROSSING_OFF_X_MAGLEV,
			SPR_TUNNEL_ENTRY_REAR_MAGLEV
		},

		/* GUI sprites */
		{ 0x4EB, 0x4EC, 0x4EE, 0x4ED,
			SPR_OPENTTD_BASE + 2, SPR_OPENTTD_BASE + 13, 0x980, SPR_OPENTTD_BASE + 29 },

		/* strings */
		{ STR_100C_MAGLEV_CONSTRUCTION },

		/* Offset of snow tiles */
		SPR_MGLV_SNOW_OFFSET,

		/* Compatible Railtypes */
		(1 << RAILTYPE_MAGLEV),

		/* main offset */
		164,
	},
};

