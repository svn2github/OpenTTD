#ifndef DEPOT_H
#define DEPOT_H

#include "pool.h"
#include "tile.h"

struct Depot {
	TileIndex xy;
	uint16 town_index;
	uint16 index;
};

extern MemoryPool _depot_pool;

/**
 * Get the pointer to the depot with index 'index'
 */
static inline Depot *GetDepot(uint index)
{
	return (Depot*)GetItemFromPool(&_depot_pool, index);
}

/**
 * Get the current size of the DepotPool
 */
static inline uint16 GetDepotPoolSize(void)
{
	return _depot_pool.total_items;
}

static inline bool IsDepotIndex(uint index)
{
	return index < GetDepotPoolSize();
}

#define FOR_ALL_DEPOTS_FROM(d, start) for (d = GetDepot(start); d != NULL; d = (d->index + 1 < GetDepotPoolSize()) ? GetDepot(d->index + 1) : NULL)
#define FOR_ALL_DEPOTS(d) FOR_ALL_DEPOTS_FROM(d, 0)

#define MIN_SERVINT_PERCENT  5
#define MAX_SERVINT_PERCENT 90
#define MIN_SERVINT_DAYS    30
#define MAX_SERVINT_DAYS   800

VARDEF TileIndex _last_built_train_depot_tile;
VARDEF TileIndex _last_built_road_depot_tile;
VARDEF TileIndex _last_built_aircraft_depot_tile;
VARDEF TileIndex _last_built_ship_depot_tile;

/**
 * Check if a depot really exists.
 */
static inline bool IsValidDepot(const Depot* depot)
{
	return depot->xy != 0; /* XXX: Replace by INVALID_TILE someday */
}

/**
 * Check if a tile is a depot of the given type.
 */
static inline bool IsTileDepotType(TileIndex tile, TransportType type)
{
	switch(type)
	{
		case TRANSPORT_RAIL:
			return IsTileType(tile, MP_RAILWAY) && (_map5[tile] & 0xFC) == 0xC0;
			break;
		case TRANSPORT_ROAD:
			return IsTileType(tile, MP_STREET) && (_map5[tile] & 0xF0) == 0x20;
			break;
		case TRANSPORT_WATER:
			return IsTileType(tile, MP_WATER) && (_map5[tile] & ~3) == 0x80;
			break;
		default:
			assert(0);
			return false;
	}
}

/**
 * Returns the direction the exit of the depot on the given tile is facing.
 */
static inline uint GetDepotDirection(TileIndex tile, TransportType type)
{
	assert(IsTileDepotType(tile, type));

	switch (type)
	{
		case TRANSPORT_RAIL:
		case TRANSPORT_ROAD:
			/* Rail and road store a diagonal direction in bits 0 and 1 */
			return _map5[tile] & 3;
		case TRANSPORT_WATER:
			/* Water is stubborn, it stores the directions in a different order. */
			switch (_map5[tile] & 3) {
				case 0: return 0;
				case 1: return 2;
				case 2: return 3;
				case 3: return 1;
			}
		default:
			return 0; /* Not reached */
	}
}

Depot *GetDepotByTile(uint tile);
void InitializeDepot(void);
Depot *AllocateDepot(void);
void DoDeleteDepot(uint tile);

#endif /* DEPOT_H */
