/* $Id$ */

/** @file elrail_data.h Stores all the data for overhead wire and pylon drawing.
 *  @see elrail.c */

#ifndef ELRAIL_DATA_H
#define ELRAIL_DATA_H

/** Tile Location group.
 * This defines whether the X and or Y coordinate of a tile is even */
enum TLG {
	XEVEN_YEVEN = 0,
	XEVEN_YODD  = 1,
	XODD_YEVEN  = 2,
	XODD_YODD   = 3,
	TLG_END
};

/** When determining the pylon configuration on the edge, two tiles are taken
 * into account: the tile being drawn itself (the home tile, the one in
 * ti->tile), and the neighbouring tile */
enum TileSource {
	TS_HOME      = 0,
	TS_NEIGHBOUR = 1,

	TS_END
};

enum {
	NUM_TRACKS_AT_PCP = 6
};

/** Which PPPs are possible at all on a given PCP */
static byte AllowedPPPonPCP[DIAGDIR_END] = {
	1 << DIR_N | 1 << DIR_E  | 1 << DIR_SE | 1 << DIR_S | 1 << DIR_W  | 1 << DIR_NW,
	1 << DIR_N | 1 << DIR_NE | 1 << DIR_E  | 1 << DIR_S | 1 << DIR_SW | 1 << DIR_W,
	1 << DIR_N | 1 << DIR_E  | 1 << DIR_SE | 1 << DIR_S | 1 << DIR_W  | 1 << DIR_NW,
	1 << DIR_N | 1 << DIR_NE | 1 << DIR_E  | 1 << DIR_S | 1 << DIR_SW | 1 << DIR_W,
};

/** Which of the PPPs are inside the tile. For the two PPPs on the tile border
 * the following system is used: if you rotate the PCP so that it is in the
 * north, the eastern PPP belongs to the tile. */
static byte OwnedPPPonPCP[DIAGDIR_END] = {
	1 << DIR_SE | 1 << DIR_S  | 1 << DIR_SW | 1 << DIR_W,
	1 << DIR_N  | 1 << DIR_SW | 1 << DIR_W  | 1 << DIR_NW,
	1 << DIR_N  | 1 << DIR_NE | 1 << DIR_E  | 1 << DIR_NW,
	1 << DIR_NE | 1 << DIR_E  | 1 << DIR_SE | 1 << DIR_S
};

/** Maps a track bit onto two PCP positions */
static const DiagDirection PCPpositions[TRACK_END][2] = {
	{DIAGDIR_NE, DIAGDIR_SW}, /* X */
	{DIAGDIR_SE, DIAGDIR_NW}, /* Y */
	{DIAGDIR_NW, DIAGDIR_NE}, /* UPPER */
	{DIAGDIR_SE, DIAGDIR_SW}, /* LOWER */
	{DIAGDIR_SW, DIAGDIR_NW}, /* LEFT */
	{DIAGDIR_NE, DIAGDIR_SE}, /* RIGHT */
};

#define PCP_NOT_ON_TRACK 0xFF
/** Preferred points of each trackbit. Those are the ones perpendicular to the
 * track, plus the point in extension of the track (to mark end-of-track). PCPs
 * which are not on either end of the track are fully preferred.
 * @see PCPpositions */
static byte PreferredPPPofTrackAtPCP[TRACK_END][DIAGDIR_END] = {
	{    /* X */
		1 << DIR_NE | 1 << DIR_SE | 1 << DIR_NW, /* NE */
		PCP_NOT_ON_TRACK,                        /* SE */
		1 << DIR_SE | 1 << DIR_SW | 1 << DIR_NW, /* SW */
		PCP_NOT_ON_TRACK                         /* NE */
	}, { /* Y */
		PCP_NOT_ON_TRACK,
		1 << DIR_NE | 1 << DIR_SE | 1 << DIR_SW,
		PCP_NOT_ON_TRACK,
		1 << DIR_SW | 1 << DIR_NW | 1 << DIR_NE
	}, { /* UPPER */
		1 << DIR_E | 1 << DIR_N | 1 << DIR_S,
		PCP_NOT_ON_TRACK,
		PCP_NOT_ON_TRACK,
		1 << DIR_W | 1 << DIR_N | 1 << DIR_S
	}, { /* LOWER */
		PCP_NOT_ON_TRACK,
		1 << DIR_E | 1 << DIR_N | 1 << DIR_S,
		1 << DIR_W | 1 << DIR_N | 1 << DIR_S,
		PCP_NOT_ON_TRACK
	}, { /* LEFT */
		PCP_NOT_ON_TRACK,
		PCP_NOT_ON_TRACK,
		1 << DIR_S | 1 << DIR_E | 1 << DIR_W,
		1 << DIR_N | 1 << DIR_E | 1 << DIR_W
	}, { /* RIGHT */
		1 << DIR_N | 1 << DIR_E | 1 << DIR_W,
		1 << DIR_S | 1 << DIR_E | 1 << DIR_W,
		PCP_NOT_ON_TRACK,
		PCP_NOT_ON_TRACK
	}
};
#undef PCP_NOT_ON_TRACK


#define NUM_IGNORE_GROUPS 3
#define IGNORE_NONE 0xFF
/** In case we have a staight line, we place pylon only every two tiles,
 * so there are certain tiles which we ignore. A straight line is found if
 * we have exactly two PPPs. */
static byte IgnoredPCP[NUM_IGNORE_GROUPS][TLG_END][DIAGDIR_END] = {
	{   /* Ignore group 1, X and Y tracks */
		{     /* X even, Y even */
			IGNORE_NONE,
			1 << DIR_NE | 1 << DIR_SW,
			1 << DIR_NW | 1 << DIR_SE,
			IGNORE_NONE
		}, { /* X even, Y odd  */
			IGNORE_NONE,
			IGNORE_NONE,
			1 << DIR_NW | 1 << DIR_SE,
			1 << DIR_NE | 1 << DIR_SW
		}, { /* X odd,  Y even */
			1 << DIR_NW | 1 << DIR_SE,
			1 << DIR_NE | 1 << DIR_SW,
			IGNORE_NONE,
			IGNORE_NONE
		}, { /* X odd,  Y odd  */
			1 << DIR_NW | 1 << DIR_SE,
			IGNORE_NONE,
			IGNORE_NONE,
			1 << DIR_NE | 1 << DIR_SW
		}
	},
	{   /* Ignore group 2, LEFT and RIGHT tracks */
		{
			1 << DIR_E | 1 << DIR_W,
			IGNORE_NONE,
			IGNORE_NONE,
			1 << DIR_E | 1 << DIR_W
		}, {
			IGNORE_NONE,
			1 << DIR_E | 1 << DIR_W,
			1 << DIR_E | 1 << DIR_W,
			IGNORE_NONE
		}, {
			IGNORE_NONE,
			1 << DIR_E | 1 << DIR_W,
			1 << DIR_E | 1 << DIR_W,
			IGNORE_NONE
		}, {
			1 << DIR_E | 1 << DIR_W,
			IGNORE_NONE,
			IGNORE_NONE,
			1 << DIR_E | 1 << DIR_W
		}
	},
	{   /* Ignore group 3, UPPER and LOWER tracks */
		{
			1 << DIR_N | 1 << DIR_S,
			1 << DIR_N | 1 << DIR_S,
			IGNORE_NONE,
			IGNORE_NONE
		}, {
			IGNORE_NONE,
			IGNORE_NONE,
			1 << DIR_N | 1 << DIR_S,
			1 << DIR_N | 1 << DIR_S
		}, {
			IGNORE_NONE,
			IGNORE_NONE,
			1 << DIR_N | 1 << DIR_S ,
			1 << DIR_N | 1 << DIR_S
		}, {
			1 << DIR_N | 1 << DIR_S,
			1 << DIR_N | 1 << DIR_S,
			IGNORE_NONE,
			IGNORE_NONE
		}
	}
};

#undef NO_IGNORE

/** Which pylons can definately NOT be built */
static byte DisallowedPPPofTrackAtPCP[TRACK_END][DIAGDIR_END] = {
	{1 << DIR_SW | 1 << DIR_NE, 0,           1 << DIR_SW | 1 << DIR_NE, 0          }, /* X */
	{0,           1 << DIR_NW | 1 << DIR_SE, 0,           1 << DIR_NW | 1 << DIR_SE}, /* Y */
	{1 << DIR_W | 1 << DIR_E,  0,           0,           1 << DIR_W | 1 << DIR_E }, /* UPPER */
	{0,           1 << DIR_W | 1 << DIR_E,  1 << DIR_W | 1 << DIR_E,  0          }, /* LOWER */
	{0,           0,           1 << DIR_S | 1 << DIR_N,  1 << DIR_N | 1 << DIR_S }, /* LEFT */
	{1 << DIR_S | 1 << DIR_N,  1 << DIR_S | 1 << DIR_N,  0,           0,         }, /* RIGHT */
};

/* This array stores which track bits can meet at a tile edge */
static const Track TracksAtPCP[DIAGDIR_END][NUM_TRACKS_AT_PCP] = {
	{TRACK_X, TRACK_X, TRACK_UPPER, TRACK_LOWER, TRACK_LEFT, TRACK_RIGHT},
	{TRACK_Y, TRACK_Y, TRACK_UPPER, TRACK_LOWER, TRACK_LEFT, TRACK_RIGHT},
	{TRACK_X, TRACK_X, TRACK_UPPER, TRACK_LOWER, TRACK_LEFT, TRACK_RIGHT},
	{TRACK_Y, TRACK_Y, TRACK_UPPER, TRACK_LOWER, TRACK_LEFT, TRACK_RIGHT},
};

/* takes each of the 6 track bits from the array above and
 * assigns it to the home tile or neighbour tile */
static const TileSource TrackSourceTile[DIAGDIR_END][NUM_TRACKS_AT_PCP] = {
	{TS_HOME, TS_NEIGHBOUR, TS_HOME     , TS_NEIGHBOUR, TS_NEIGHBOUR, TS_HOME     },
	{TS_HOME, TS_NEIGHBOUR, TS_NEIGHBOUR, TS_HOME     , TS_NEIGHBOUR, TS_HOME     },
	{TS_HOME, TS_NEIGHBOUR, TS_NEIGHBOUR, TS_HOME     , TS_HOME     , TS_NEIGHBOUR},
	{TS_HOME, TS_NEIGHBOUR, TS_HOME     , TS_NEIGHBOUR, TS_HOME     , TS_NEIGHBOUR},
};

/* Several PPPs maybe exist, here they are sorted in order of preference. */
static const Direction PPPorder[DIAGDIR_END][TLG_END][DIR_END] = {    /*  X  -  Y  */
	{   /* PCP 0 */
		{DIR_NE, DIR_NW, DIR_SE, DIR_SW, DIR_N, DIR_E, DIR_S, DIR_W}, /* evn - evn */
		{DIR_NE, DIR_SE, DIR_SW, DIR_NW, DIR_S, DIR_W, DIR_N, DIR_E}, /* evn - odd */
		{DIR_SW, DIR_NW, DIR_NE, DIR_SE, DIR_S, DIR_W, DIR_N, DIR_E}, /* odd - evn */
		{DIR_SW, DIR_SE, DIR_NE, DIR_NW, DIR_N, DIR_E, DIR_S, DIR_W}, /* odd - odd */
	}, {/* PCP 1 */
		{DIR_NE, DIR_NW, DIR_SE, DIR_SW, DIR_S, DIR_E, DIR_N, DIR_W}, /* evn - evn */
		{DIR_NE, DIR_SE, DIR_SW, DIR_NW, DIR_N, DIR_W, DIR_S, DIR_E}, /* evn - odd */
		{DIR_SW, DIR_NW, DIR_NE, DIR_SE, DIR_N, DIR_W, DIR_S, DIR_E}, /* odd - evn */
		{DIR_SW, DIR_SE, DIR_NE, DIR_NW, DIR_S, DIR_E, DIR_N, DIR_W}, /* odd - odd */
	}, {/* PCP 2 */
		{DIR_NE, DIR_NW, DIR_SE, DIR_SW, DIR_S, DIR_W, DIR_N, DIR_E}, /* evn - evn */
		{DIR_NE, DIR_SE, DIR_SW, DIR_NW, DIR_N, DIR_E, DIR_S, DIR_W}, /* evn - odd */
		{DIR_SW, DIR_NW, DIR_NE, DIR_SE, DIR_N, DIR_E, DIR_S, DIR_W}, /* odd - evn */
		{DIR_SW, DIR_SE, DIR_NE, DIR_NW, DIR_S, DIR_W, DIR_N, DIR_E}, /* odd - odd */
	}, {/* PCP 3 */
		{DIR_NE, DIR_NW, DIR_SE, DIR_SW, DIR_N, DIR_W, DIR_S, DIR_E}, /* evn - evn */
		{DIR_NE, DIR_SE, DIR_SW, DIR_NW, DIR_S, DIR_E, DIR_N, DIR_W}, /* evn - odd */
		{DIR_SW, DIR_NW, DIR_NE, DIR_SE, DIR_S, DIR_E, DIR_N, DIR_W}, /* odd - evn */
		{DIR_SW, DIR_SE, DIR_NE, DIR_NW, DIR_N, DIR_W, DIR_S, DIR_E}, /* odd - odd */
	}
};
/* Geometric placement of the PCP relative to the tile origin */
static const int8 x_pcp_offsets[DIAGDIR_END] = {0,  8, 16, 8};
static const int8 y_pcp_offsets[DIAGDIR_END] = {8, 16,  8, 0};
/* Geometric placement of the PPP relative to the PCP*/
static const int8 x_ppp_offsets[DIR_END] = {-2, -4, -2,  0,  2,  4,  2,  0};
static const int8 y_ppp_offsets[DIR_END] = {-2,  0,  2,  4,  2,  0, -2, -4};
/* The type of pylon to draw at each PPP */
static const SpriteID pylon_sprites[] = {
	SPR_PYLON_EW_N,
	SPR_PYLON_Y_NE,
	SPR_PYLON_NS_E,
	SPR_PYLON_X_SE,
	SPR_PYLON_EW_S,
	SPR_PYLON_Y_SW,
	SPR_PYLON_NS_W,
	SPR_PYLON_X_NW
};

struct SortableSpriteStruct {
	SpriteID image;
	int8 x_offset;
	int8 y_offset;
	int8 x_size;
	int8 y_size;
	int8 z_size;
	int8 z_offset;
};

enum {
	/** Distance between wire and rail */
	ELRAIL_ELEVATION = 10,
	/** Wires that a draw one level higher than the north corner. */
	ELRAIL_ELEVRAISE = ELRAIL_ELEVATION + TILE_HEIGHT
};

static const SortableSpriteStruct CatenarySpriteData[] = {
/* X direction */
	/* Flat tiles: */
		/* Wires */
	{ SPR_WIRE_X_SW,          0,  7, 15,  1,  1, ELRAIL_ELEVATION }, //! 0: Wire in X direction, pylon on the SW end only
	{ SPR_WIRE_X_NE,          0,  7, 15,  1,  1, ELRAIL_ELEVATION }, //! 1: Wire in X direction, pylon on the NE end
	{ SPR_WIRE_X_SHORT,       0,  7, 15,  1,  1, ELRAIL_ELEVATION }, //! 2: Wire in X direction, pylon on both ends

	/* "up" tiles */
		/* Wires */
	{ SPR_WIRE_X_SW_UP,       0,  7, 15,  8,  1, ELRAIL_ELEVRAISE }, //! 3: Wire in X pitch up, pylon on the SW end only
	{ SPR_WIRE_X_NE_UP,       0,  7, 15,  8,  1, ELRAIL_ELEVRAISE }, //! 4: Wire in X pitch up, pylon on the NE end
	{ SPR_WIRE_X_SHORT_UP,    0,  7, 15,  8,  1, ELRAIL_ELEVRAISE }, //! 5: Wire in X pitch up, pylon on both ends

	/* "down" tiles */
		/* Wires */
	{ SPR_WIRE_X_SW_DOWN,     0,  7, 15,  8,  1, ELRAIL_ELEVATION }, //! 6: Wire in X pitch down, pylon on the SW end
	{ SPR_WIRE_X_NE_DOWN,     0,  7, 15,  8,  1, ELRAIL_ELEVATION }, //! 7: Wire in X pitch down, pylon on the NE end
	{ SPR_WIRE_X_SHORT_DOWN,  0,  7, 15,  8,  1, ELRAIL_ELEVATION }, //! 8: Wire in X pitch down, pylon on both ends


/* Y direction */
	/* Flat tiles: */
		/* Wires */
	{ SPR_WIRE_Y_SE,          7,  0,  1, 15,  1, ELRAIL_ELEVATION }, //! 9: Wire in Y direction, pylon on the SE end only
	{ SPR_WIRE_Y_NW,          7,  0,  1, 15,  1, ELRAIL_ELEVATION }, //!10: Wire in Y direction, pylon on the NW end
	{ SPR_WIRE_Y_SHORT,       7,  0,  1, 15,  1, ELRAIL_ELEVATION }, //!11: Wire in Y direction, pylon on both ends

	/* "up" tiles */
		/* Wires */
	{ SPR_WIRE_Y_SE_UP,       7,  0,  8, 15,  1, ELRAIL_ELEVRAISE }, //!12: Wire in Y pitch up, pylon on the SE end only
	{ SPR_WIRE_Y_NW_UP,       7,  0,  8, 15,  1, ELRAIL_ELEVRAISE }, //!13: Wire in Y pitch up, pylon on the NW end
	{ SPR_WIRE_Y_SHORT_UP,    7,  0,  8, 15,  1, ELRAIL_ELEVRAISE }, //!14: Wire in Y pitch up, pylon on both ends

	/* "down" tiles */
		/* Wires */
	{ SPR_WIRE_Y_SE_DOWN,     7,  0,  8, 15,  1, ELRAIL_ELEVATION }, //!15: Wire in Y pitch down, pylon on the SE end
	{ SPR_WIRE_Y_NW_DOWN,     7,  0,  8, 15,  1, ELRAIL_ELEVATION }, //!16: Wire in Y pitch down, pylon on the NW end
	{ SPR_WIRE_Y_SHORT_DOWN,  7,  0,  8, 15,  1, ELRAIL_ELEVATION }, //!17: Wire in Y pitch down, pylon on both ends

/* NS Direction */
	{ SPR_WIRE_NS_SHORT,      8,  0,  8,  8,  1, ELRAIL_ELEVATION }, //!18: LEFT  trackbit wire, pylon on both ends
	{ SPR_WIRE_NS_SHORT,      0,  8,  8,  8,  1, ELRAIL_ELEVATION }, //!19: RIGHT trackbit wire, pylon on both ends

	{ SPR_WIRE_NS_N,          8,  0,  8,  8,  1, ELRAIL_ELEVATION }, //!20: LEFT  trackbit wire, pylon on N end
	{ SPR_WIRE_NS_N,          0,  8,  8,  8,  1, ELRAIL_ELEVATION }, //!21: RIGHT trackbit wire, pylon on N end

	{ SPR_WIRE_NS_S,          8,  0,  8,  8,  1, ELRAIL_ELEVATION }, //!22: LEFT  trackbit wire, pylon on S end
	{ SPR_WIRE_NS_S,          0,  8,  8,  8,  1, ELRAIL_ELEVATION }, //!23: RIGHT trackbit wire, pylon on S end

/* EW Direction */
	{ SPR_WIRE_EW_SHORT,      7,  0,  1,  1,  1, ELRAIL_ELEVATION }, //!24: UPPER trackbit wire, pylon on both ends
	{ SPR_WIRE_EW_SHORT,     15,  8,  3,  3,  1, ELRAIL_ELEVATION }, //!25: LOWER trackbit wire, pylon on both ends

	{ SPR_WIRE_EW_W,          7,  0,  1,  1,  1, ELRAIL_ELEVATION }, //!28: UPPER trackbit wire, pylon on both ends
	{ SPR_WIRE_EW_W,         15,  8,  3,  3,  1, ELRAIL_ELEVATION }, //!29: LOWER trackbit wire, pylon on both ends

	{ SPR_WIRE_EW_E,          7,  0,  1,  1,  1, ELRAIL_ELEVATION }, //!32: UPPER trackbit wire, pylon on both ends
	{ SPR_WIRE_EW_E,         15,  8,  3,  3,  1, ELRAIL_ELEVATION }  //!33: LOWER trackbit wire, pylon on both ends
};

static const SortableSpriteStruct CatenarySpriteData_Depot[] = {
	{ SPR_WIRE_DEPOT_NE,      0,  7, 15,  1,  1, ELRAIL_ELEVATION }, //! Wire for NE depot exit
	{ SPR_WIRE_DEPOT_SE,      7,  0,  1, 15,  1, ELRAIL_ELEVATION }, //! Wire for SE depot exit
	{ SPR_WIRE_DEPOT_SW,      0,  7, 15,  1,  1, ELRAIL_ELEVATION }, //! Wire for SW depot exit
	{ SPR_WIRE_DEPOT_NW,      7,  0,  1, 15,  1, ELRAIL_ELEVATION }  //! Wire for NW depot exit
};

static const SortableSpriteStruct CatenarySpriteData_Tunnel[] = {
	{ SPR_WIRE_TUNNEL_NE,      0,  7, 15,  1,  1, ELRAIL_ELEVATION }, //! Wire for NE tunnel exit
	{ SPR_WIRE_TUNNEL_SE,      7,  0,  1, 15,  1, ELRAIL_ELEVATION }, //! Wire for SE tunnel exit
	{ SPR_WIRE_TUNNEL_SW,      0,  7, 15,  1,  1, ELRAIL_ELEVATION }, //! Wire for SW tunnel exit
	{ SPR_WIRE_TUNNEL_NW,      7,  0,  1, 15,  1, ELRAIL_ELEVATION }  //! Wire for NW tunnel exit
};


/** Refers to a certain element of the catenary.
 * Identifiers for Wires:
 * <ol><li>Direction of the wire</li>
 * <li>Slope of the tile for diagonals, placement inside the track for horiz/vertical pieces</li>
 * <li>Place where a pylon shoule be</li></ol>
 * Identifiers for Pylons:
 * <ol><li>Direction of the wire</li>
 * <li>Slope of the tile</li>
 * <li>Position of the Pylon relative to the track</li>
 * <li>Position of the Pylon inside the tile</li></ol>
 */
enum CatenarySprite {
	WIRE_X_FLAT_SW,
	WIRE_X_FLAT_NE,
	WIRE_X_FLAT_BOTH,

	WIRE_X_UP_SW,
	WIRE_X_UP_NE,
	WIRE_X_UP_BOTH,

	WIRE_X_DOWN_SW,
	WIRE_X_DOWN_NE,
	WIRE_X_DOWN_BOTH,

	WIRE_Y_FLAT_SE,
	WIRE_Y_FLAT_NW,
	WIRE_Y_FLAT_BOTH,

	WIRE_Y_UP_SE,
	WIRE_Y_UP_NW,
	WIRE_Y_UP_BOTH,

	WIRE_Y_DOWN_SE,
	WIRE_Y_DOWN_NW,
	WIRE_Y_DOWN_BOTH,

	WIRE_NS_W_BOTH,
	WIRE_NS_E_BOTH,

	WIRE_NS_W_N,
	WIRE_NS_E_N,

	WIRE_NS_W_S,
	WIRE_NS_E_S,

	WIRE_EW_N_BOTH,
	WIRE_EW_S_BOTH,

	WIRE_EW_N_W,
	WIRE_EW_S_W,

	WIRE_EW_N_E,
	WIRE_EW_S_E,

	INVALID_CATENARY = 0xFF
};

/* Selects a Wire (with white and grey ends) depending on whether:
 * a) none (should never happen)
 * b) the first
 * c) the second
 * d) both
 * PCP exists.*/
static const CatenarySprite Wires[5][TRACK_END][4] = {
	{ /* Tileh == 0 */
		{INVALID_CATENARY, WIRE_X_FLAT_NE,   WIRE_X_FLAT_SW,   WIRE_X_FLAT_BOTH},
		{INVALID_CATENARY, WIRE_Y_FLAT_SE,   WIRE_Y_FLAT_NW,   WIRE_Y_FLAT_BOTH},
		{INVALID_CATENARY, WIRE_EW_N_W,      WIRE_EW_N_E,      WIRE_EW_N_BOTH},
		{INVALID_CATENARY, WIRE_EW_S_E,      WIRE_EW_S_W,      WIRE_EW_S_BOTH},
		{INVALID_CATENARY, WIRE_NS_W_S,      WIRE_NS_W_N,      WIRE_NS_W_BOTH},
		{INVALID_CATENARY, WIRE_NS_E_N,      WIRE_NS_E_S,      WIRE_NS_E_BOTH},
	}, { /* Tileh == 3 */
		{INVALID_CATENARY, WIRE_X_UP_NE,     WIRE_X_UP_SW,     WIRE_X_UP_BOTH},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
	}, { /* Tileh == 6 */
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, WIRE_Y_UP_SE,     WIRE_Y_UP_NW,     WIRE_Y_UP_BOTH},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
	}, { /* Tileh == 9 */
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, WIRE_Y_DOWN_SE,   WIRE_Y_DOWN_NW,   WIRE_Y_DOWN_BOTH},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
	}, { /* Tileh == 12 */
		{INVALID_CATENARY, WIRE_X_DOWN_NE,   WIRE_X_DOWN_SW,   WIRE_X_DOWN_BOTH},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
		{INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY, INVALID_CATENARY},
	}
};

#endif /* ELRAIL_DATA_H */
