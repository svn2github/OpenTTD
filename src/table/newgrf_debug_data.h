/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file newgrf_debug_data.cpp Data 'tables' for NewGRF debugging. */

/* Helper for filling property tables */
#define NIP(prop, base, variable, type, name) { name, cpp_offsetof(base, variable), cpp_sizeof(base, variable), prop, type }
#define NIP_END() { NULL, 0, 0, 0, 0 }

/* Helper for filling callback tables */
#define NIC(cb_id, base, variable, bit) { #cb_id, cpp_offsetof(base, variable), cpp_sizeof(base, variable), bit, cb_id }
#define NIC_END() { NULL, 0, 0, 0, 0 }

/* Helper for filling variable tables */
#define NIV(var, name) { name, var }
#define NIV_END() { NULL, 0 }


/*** NewGRF Vehicles ***/

#define NICV(cb_id, bit) NIC(cb_id, Engine, info.callback_mask, bit)
static const NICallback _nic_vehicles[] = {
	NICV(CBID_TRAIN_WAGON_POWER,             CBM_TRAIN_WAGON_POWER),
	NICV(CBID_VEHICLE_LENGTH,                CBM_VEHICLE_LENGTH),
	NICV(CBID_VEHICLE_LOAD_AMOUNT,           CBM_VEHICLE_LOAD_AMOUNT),
	NICV(CBID_VEHICLE_REFIT_CAPACITY,        CBM_VEHICLE_REFIT_CAPACITY),
	NICV(CBID_VEHICLE_ARTIC_ENGINE,          CBM_VEHICLE_ARTIC_ENGINE),
	NICV(CBID_VEHICLE_CARGO_SUFFIX,          CBM_VEHICLE_CARGO_SUFFIX),
	NICV(CBID_TRAIN_ALLOW_WAGON_ATTACH,      CBM_NO_BIT),
	NICV(CBID_VEHICLE_ADDITIONAL_TEXT,       CBM_NO_BIT),
	NICV(CBID_VEHICLE_COLOUR_MAPPING,        CBM_VEHICLE_COLOUR_REMAP),
	NICV(CBID_VEHICLE_START_STOP_CHECK,      CBM_NO_BIT),
	NICV(CBID_VEHICLE_32DAY_CALLBACK,        CBM_NO_BIT),
	NICV(CBID_VEHICLE_SOUND_EFFECT,          CBM_VEHICLE_SOUND_EFFECT),
	NICV(CBID_VEHICLE_AUTOREPLACE_SELECTION, CBM_NO_BIT),
	NICV(CBID_VEHICLE_MODIFY_PROPERTY,       CBM_NO_BIT),
	NIC_END()
};


static const NIVariable _niv_vehicles[] = {
	NIV(0x40, "position in consist and length"),
	NIV(0x41, "position and length of chain of same vehicles"),
	NIV(0x42, "transported cargo types"),
	NIV(0x43, "player info"),
	NIV(0x44, "aircraft info"),
	NIV(0x45, "curvature info"),
	NIV(0x46, "motion counter"),
	NIV(0x47, "vehicle cargo info"),
	NIV(0x48, "vehicle type info"),
	NIV(0x49, "year of construction"),
	NIV(0x60, "count vehicle id occurrences"),
	NIV_END()
};

class NIHVehicle : public NIHelper {
	bool IsInspectable(uint index) const                 { return Engine::Get(Vehicle::Get(index)->engine_type)->grffile != NULL; }
	uint GetParent(uint index) const                     { const Vehicle *first = Vehicle::Get(index)->First(); return GetInspectWindowNumber(GetGrfSpecFeature(first->type), first->index); }
	const void *GetInstance(uint index)const             { return Vehicle::Get(index); }
	const void *GetSpec(uint index) const                { return Engine::Get(Vehicle::Get(index)->engine_type); }
	void SetStringParameters(uint index) const           { this->SetSimpleStringParameters(STR_VEHICLE_NAME, index); }
	void Resolve(ResolverObject *ro, uint32 index) const { extern void GetVehicleResolver(ResolverObject *ro, uint index); GetVehicleResolver(ro, index); }
};

static const NIFeature _nif_vehicle = {
	NULL,
	_nic_vehicles,
	_niv_vehicles,
	new NIHVehicle(),
	0,
	0
};


/*** NewGRF station (tiles) ***/

#define NICS(cb_id, bit) NIC(cb_id, StationSpec, callback_mask, bit)
static const NICallback _nic_stations[] = {
	NICS(CBID_STATION_AVAILABILITY,     CBM_STATION_AVAIL),
	NICS(CBID_STATION_SPRITE_LAYOUT,    CBM_NO_BIT),
	NICS(CBID_STATION_TILE_LAYOUT,      CBM_STATION_SPRITE_LAYOUT),
	NICS(CBID_STATION_ANIM_START_STOP,  CBM_NO_BIT),
	NICS(CBID_STATION_ANIM_NEXT_FRAME,  CBM_STATION_ANIMATION_NEXT_FRAME),
	NICS(CBID_STATION_ANIMATION_SPEED,  CBM_STATION_ANIMATION_SPEED),
	NICS(CBID_STATION_LAND_SLOPE_CHECK, CBM_STATION_SLOPE_CHECK),
	NIC_END()
};

static const NIVariable _niv_stations[] = {
	NIV(0x40, "platform info and relative position"),
	NIV(0x41, "platform info and relative position for individually built sections"),
	NIV(0x42, "terrain and track type"),
	NIV(0x43, "player info"),
	NIV(0x44, "path signalling info"),
	NIV(0x45, "rail continuation info"),
	NIV(0x46, "platform info and relative position from middle"),
	NIV(0x47, "platform info and relative position from middle for individually built sections"),
	NIV(0x48, "bitmask of accepted cargoes"),
	NIV(0x49, "platform info and relative position of same-direction section"),
	NIV(0x4A, "current animation frame"),
	NIV(0x60, "amount of cargo waiting"),
	NIV(0x61, "time since last cargo pickup"),
	NIV(0x62, "rating of cargo"),
	NIV(0x63, "time spent on route"),
	NIV(0x64, "information about last vehicle picking cargo up"),
	NIV(0x65, "amount of cargo acceptance"),
	NIV(0x66, "animation frame of nearby tile"),
	NIV(0x67, "land info of nearby tiles"),
	NIV(0x68, "station info of nearby tiles"),
	NIV(0x69, "information about cargo accepted in the past"),
	NIV_END()
};

class NIHStation : public NIHelper {
	bool IsInspectable(uint index) const                 { return GetStationSpec(index) != NULL; }
	uint GetParent(uint index) const                     { return GetInspectWindowNumber(GSF_FAKE_TOWNS, Station::GetByTile(index)->town->index); }
	const void *GetInstance(uint index)const             { return NULL; }
	const void *GetSpec(uint index) const                { return GetStationSpec(index); }
	void SetStringParameters(uint index) const           { this->SetObjectAtStringParameters(STR_STATION_NAME, GetStationIndex(index), index); }
	void Resolve(ResolverObject *ro, uint32 index) const { extern void GetStationResolver(ResolverObject *ro, uint index); GetStationResolver(ro, index); }
};

static const NIFeature _nif_station = {
	NULL,
	_nic_stations,
	_niv_stations,
	new NIHStation(),
	0,
	0
};


/*** NewGRF house tiles ***/

#define NICH(cb_id, bit) NIC(cb_id, HouseSpec, callback_mask, bit)
static const NICallback _nic_house[] = {
	NICH(CBID_HOUSE_ALLOW_CONSTRUCTION,        CBM_HOUSE_ALLOW_CONSTRUCTION),
	NICH(CBID_HOUSE_ANIMATION_NEXT_FRAME,      CBM_HOUSE_ANIMATION_NEXT_FRAME),
	NICH(CBID_HOUSE_ANIMATION_START_STOP,      CBM_HOUSE_ANIMATION_START_STOP),
	NICH(CBID_HOUSE_CONSTRUCTION_STATE_CHANGE, CBM_HOUSE_CONSTRUCTION_STATE_CHANGE),
	NICH(CBID_HOUSE_COLOUR,                    CBM_HOUSE_COLOUR),
	NICH(CBID_HOUSE_CARGO_ACCEPTANCE,          CBM_HOUSE_CARGO_ACCEPTANCE),
	NICH(CBID_HOUSE_ANIMATION_SPEED,           CBM_HOUSE_ANIMATION_SPEED),
	NICH(CBID_HOUSE_DESTRUCTION,               CBM_HOUSE_DESTRUCTION),
	NICH(CBID_HOUSE_ACCEPT_CARGO,              CBM_HOUSE_ACCEPT_CARGO),
	NICH(CBID_HOUSE_PRODUCE_CARGO,             CBM_HOUSE_PRODUCE_CARGO),
	NICH(CBID_HOUSE_DENY_DESTRUCTION,          CBM_HOUSE_DENY_DESTRUCTION),
	NICH(CBID_HOUSE_WATCHED_CARGO_ACCEPTED,    CBM_NO_BIT),
	NICH(CBID_HOUSE_CUSTOM_NAME,               CBM_NO_BIT),
	NICH(CBID_HOUSE_DRAW_FOUNDATIONS,          CBM_HOUSE_DRAW_FOUNDATIONS),
	NICH(CBID_HOUSE_AUTOSLOPE,                 CBM_HOUSE_AUTOSLOPE),
	NIC_END()
};

static const NIVariable _niv_house[] = {
	NIV(0x40, "construction state of tile and pseudo-random value"),
	NIV(0x41, "age of building in years"),
	NIV(0x42, "town zone"),
	NIV(0x43, "terrain type"),
	NIV(0x44, "building counts"),
	NIV(0x45, "town expansion bits"),
	NIV(0x46, "current animation frame"),
	NIV(0x47, "xy coordinate of the building"),
	NIV(0x60, "other building counts (old house type)"),
	NIV(0x61, "other building counts (new house type)"),
	NIV(0x62, "land info of nearby tiles"),
	NIV(0x63, "current animation frame of nearby house tile"),
	NIV(0x64, "cargo acceptance history of nearby stations"),
	NIV(0x65, "distance of nearest house matching a given criterion"),
	NIV(0x66, "class and ID of nearby house tile"),
	NIV(0x67, "GRFID of nearby house tile"),
	NIV_END()
};

class NIHHouse : public NIHelper {
	bool IsInspectable(uint index) const                 { return HouseSpec::Get(GetHouseType(index))->grffile != NULL; }
	uint GetParent(uint index) const                     { return GetInspectWindowNumber(GSF_FAKE_TOWNS, GetTownIndex(index)); }
	const void *GetInstance(uint index)const             { return NULL; }
	const void *GetSpec(uint index) const                { return HouseSpec::Get(GetHouseType(index)); }
	void SetStringParameters(uint index) const           { this->SetObjectAtStringParameters(STR_TOWN_NAME, GetTownIndex(index), index); }
	void Resolve(ResolverObject *ro, uint32 index) const { extern void GetHouseResolver(ResolverObject *ro, uint index); GetHouseResolver(ro, index); }
};

static const NIFeature _nif_house = {
	NULL,
	_nic_house,
	_niv_house,
	new NIHHouse(),
	0,
	0
};


/*** NewGRF industry tiles ***/

#define NICIT(cb_id, bit) NIC(cb_id, IndustryTileSpec, callback_mask, bit)
static const NICallback _nic_industrytiles[] = {
	NICIT(CBID_INDTILE_ANIM_START_STOP,  CBM_NO_BIT),
	NICIT(CBID_INDTILE_ANIM_NEXT_FRAME,  CBM_INDT_ANIM_NEXT_FRAME),
	NICIT(CBID_INDTILE_ANIMATION_SPEED,  CBM_INDT_ANIM_SPEED),
	NICIT(CBID_INDTILE_CARGO_ACCEPTANCE, CBM_INDT_CARGO_ACCEPTANCE),
	NICIT(CBID_INDTILE_ACCEPT_CARGO,     CBM_INDT_ACCEPT_CARGO),
	NICIT(CBID_INDTILE_SHAPE_CHECK,      CBM_INDT_SHAPE_CHECK),
	NICIT(CBID_INDTILE_DRAW_FOUNDATIONS, CBM_INDT_DRAW_FOUNDATIONS),
	NICIT(CBID_INDTILE_AUTOSLOPE,        CBM_INDT_AUTOSLOPE),
	NIC_END()
};

static const NIVariable _niv_industrytiles[] = {
	NIV(0x40, "construction state of tile"),
	NIV(0x41, "ground type"),
	NIV(0x42, "current town zone in nearest town"),
	NIV(0x43, "relative position"),
	NIV(0x44, "animation frame"),
	NIV(0x60, "land info of nearby tiles"),
	NIV(0x61, "animation stage of nearby tiles"),
	NIV(0x62, "get industry or airport tile ID at offset"),
	NIV_END()
};

class NIHIndustryTile : public NIHelper {
	bool IsInspectable(uint index) const                 { return GetIndustryTileSpec(GetIndustryGfx(index))->grf_prop.grffile != NULL; }
	uint GetParent(uint index) const                     { return GetInspectWindowNumber(GSF_INDUSTRIES, GetIndustryIndex(index)); }
	const void *GetInstance(uint index)const             { return NULL; }
	const void *GetSpec(uint index) const                { return GetIndustryTileSpec(GetIndustryGfx(index)); }
	void SetStringParameters(uint index) const           { this->SetObjectAtStringParameters(STR_INDUSTRY_NAME, GetIndustryIndex(index), index); }
	void Resolve(ResolverObject *ro, uint32 index) const { extern void GetIndustryTileResolver(ResolverObject *ro, uint index); GetIndustryTileResolver(ro, index); }
};

static const NIFeature _nif_industrytile = {
	NULL,
	_nic_industrytiles,
	_niv_industrytiles,
	new NIHIndustryTile(),
	0,
	0
};


/*** NewGRF industries ***/

static const NIProperty _nip_industries[] = {
	NIP(0x10, Industry, produced_cargo[0], NIT_CARGO, "produced cargo 0"),
	NIP(0x10, Industry, produced_cargo[1], NIT_CARGO, "produced cargo 1"),
	NIP(0x11, Industry, accepts_cargo[0],  NIT_CARGO, "accepted cargo 0"),
	NIP(0x11, Industry, accepts_cargo[1],  NIT_CARGO, "accepted cargo 1"),
	NIP(0x11, Industry, accepts_cargo[2],  NIT_CARGO, "accepted cargo 2"),
	NIP_END()
};

#define NICI(cb_id, bit) NIC(cb_id, IndustrySpec, callback_mask, bit)
static const NICallback _nic_industries[] = {
	NICI(CBID_INDUSTRY_AVAILABLE,            CBM_IND_AVAILABLE),
	NICI(CBID_INDUSTRY_LOCATION,             CBM_IND_LOCATION),
	NICI(CBID_INDUSTRY_PRODUCTION_CHANGE,    CBM_IND_PRODUCTION_CHANGE),
	NICI(CBID_INDUSTRY_MONTHLYPROD_CHANGE,   CBM_IND_MONTHLYPROD_CHANGE),
	NICI(CBID_INDUSTRY_CARGO_SUFFIX,         CBM_IND_CARGO_SUFFIX),
	NICI(CBID_INDUSTRY_FUND_MORE_TEXT,       CBM_IND_FUND_MORE_TEXT),
	NICI(CBID_INDUSTRY_WINDOW_MORE_TEXT,     CBM_IND_WINDOW_MORE_TEXT),
	NICI(CBID_INDUSTRY_SPECIAL_EFFECT,       CBM_IND_SPECIAL_EFFECT),
	NICI(CBID_INDUSTRY_REFUSE_CARGO,         CBM_IND_REFUSE_CARGO),
	NICI(CBID_INDUSTRY_DECIDE_COLOUR,        CBM_IND_DECIDE_COLOUR),
	NICI(CBID_INDUSTRY_INPUT_CARGO_TYPES,    CBM_IND_INPUT_CARGO_TYPES),
	NICI(CBID_INDUSTRY_OUTPUT_CARGO_TYPES,   CBM_IND_OUTPUT_CARGO_TYPES),
	NIC_END()
};

static const NIVariable _niv_industries[] = {
	NIV(0x40, "waiting cargo 0"),
	NIV(0x41, "waiting cargo 1"),
	NIV(0x42, "waiting cargo 2"),
	NIV(0x43, "distance to closest dry/land tile"),
	NIV(0x44, "layout number"),
	NIV(0x45, "player info"),
	NIV(0x46, "industry construction date"),
	NIV(0x60, "get industry tile ID at offset"),
	NIV(0x61, "get random tile bits at offset"),
	NIV(0x62, "land info of nearby tiles"),
	NIV(0x63, "animation stage of nearby tiles"),
	NIV(0x64, "distance on nearest industry with given type"),
	NIV(0x65, "get town zone and Manhattan distance of closest town"),
	NIV(0x66, "get square of Euclidean distance of closes town"),
	NIV(0x67, "count of industry and distance of closest instance"),
	NIV(0x68, "count of industry and distance of closest instance with layout filter"),
	NIV_END()
};

class NIHIndustry : public NIHelper {
	bool IsInspectable(uint index) const                 { return GetIndustrySpec(Industry::Get(index)->type)->grf_prop.grffile != NULL; }
	uint GetParent(uint index) const                     { return GetInspectWindowNumber(GSF_FAKE_TOWNS, Industry::Get(index)->town->index); }
	const void *GetInstance(uint index)const             { return Industry::Get(index); }
	const void *GetSpec(uint index) const                { return GetIndustrySpec(Industry::Get(index)->type); }
	void SetStringParameters(uint index) const           { this->SetSimpleStringParameters(STR_INDUSTRY_NAME, index); }
	void Resolve(ResolverObject *ro, uint32 index) const { extern void GetIndustryResolver(ResolverObject *ro, uint index); GetIndustryResolver(ro, index); }
};

static const NIFeature _nif_industry = {
	_nip_industries,
	_nic_industries,
	_niv_industries,
	new NIHIndustry(),
	cpp_lengthof(Industry, psa.storage),
	cpp_offsetof(Industry, psa.storage)
};


/*** NewGRF rail types ***/

static const NIVariable _niv_railtypes[] = {
	NIV(0x40, "terrain type"),
	NIV(0x41, "enhanced tunnels"),
	NIV(0x42, "level crossing status"),
	NIV_END()
};

class NIHRailType : public NIHelper {
	bool IsInspectable(uint index) const                 { return true; }
	uint GetParent(uint index) const                     { return UINT32_MAX; }
	const void *GetInstance(uint index)const             { return NULL; }
	const void *GetSpec(uint index) const                { return NULL; }
	void SetStringParameters(uint index) const           { this->SetObjectAtStringParameters(STR_NEWGRF_INSPECT_CAPTION_OBJECT_AT_RAIL_TYPE, INVALID_STRING_ID, index); }
	void Resolve(ResolverObject *ro, uint32 index) const { extern void GetRailTypeResolver(ResolverObject *ro, uint index); GetRailTypeResolver(ro, index); }
};

static const NIFeature _nif_railtype = {
	NULL,
	NULL,
	_niv_railtypes,
	new NIHRailType(),
	0,
	0
};


/*** NewGRF airport tiles ***/

#define NICAT(cb_id, bit) NIC(cb_id, AirportTileSpec, callback_flags, bit)
static const NICallback _nic_airporttiles[] = {
	NICAT(CBID_AIRPTILE_DRAW_FOUNDATIONS, CBM_AIRT_DRAW_FOUNDATIONS),
	NICAT(CBID_AIRPTILE_ANIM_START_STOP,  CBM_NO_BIT),
	NICAT(CBID_AIRPTILE_ANIM_NEXT_FRAME,  CBM_AIRT_ANIM_NEXT_FRAME),
	NICAT(CBID_AIRPTILE_ANIMATION_SPEED,  CBM_AIRT_ANIM_SPEED),
	NIC_END()
};

class NIHAirportTile : public NIHelper {
	bool IsInspectable(uint index) const                 { return AirportTileSpec::Get(GetAirportGfx(index))->grf_prop.grffile != NULL; }
	uint GetParent(uint index) const                     { return GetInspectWindowNumber(GSF_FAKE_TOWNS, Station::GetByTile(index)->town->index); }
	const void *GetInstance(uint index)const             { return NULL; }
	const void *GetSpec(uint index) const                { return AirportTileSpec::Get(GetAirportGfx(index)); }
	void SetStringParameters(uint index) const           { this->SetObjectAtStringParameters(STR_STATION_NAME, GetStationIndex(index), index); }
	void Resolve(ResolverObject *ro, uint32 index) const { extern void GetAirportTileTypeResolver(ResolverObject *ro, uint index); GetAirportTileTypeResolver(ro, index); }
};

static const NIFeature _nif_airporttile = {
	NULL,
	_nic_airporttiles,
	_niv_industrytiles, // Yes, they share this (at least now)
	new NIHAirportTile(),
	0,
	0
};


/*** NewGRF towns ***/

static const NIVariable _niv_towns[] = {
	NIV(0x40, "larger town effect on this town"),
	NIV(0x41, "town index"),
	NIV(0x82, "population"),
	NIV(0x94, "zone radius 0"),
	NIV(0x96, "zone radius 1"),
	NIV(0x98, "zone radius 2"),
	NIV(0x9A, "zone radius 3"),
	NIV(0x9C, "zone radius 4"),
	NIV(0xB6, "number of buildings"),
	NIV_END()
};

class NIHTown : public NIHelper {
	bool IsInspectable(uint index) const                 { return false; }
	uint GetParent(uint index) const                     { return UINT32_MAX; }
	const void *GetInstance(uint index)const             { return Town::Get(index); }
	const void *GetSpec(uint index) const                { return NULL; }
	void SetStringParameters(uint index) const           { this->SetSimpleStringParameters(STR_TOWN_NAME, index); }
	uint Resolve(uint index, uint var, uint param, bool *avail) const { return TownGetVariable(var, param, avail, Town::Get(index)); }
};

static const NIFeature _nif_town = {
	NULL,
	NULL,
	_niv_towns,
	new NIHTown(),
	0,
	0
};

/** Table with all NIFeatures. */
static const NIFeature * const _nifeatures[] = {
	&_nif_vehicle,      // GSF_TRAINS
	&_nif_vehicle,      // GSF_ROADVEHICLES
	&_nif_vehicle,      // GSF_SHIPS
	&_nif_vehicle,      // GSF_AIRCRAFT
	&_nif_station,      // GSF_STATIONS
	NULL,               // GSF_CANALS (no callbacks/action2 implemented)
	NULL,               // GSF_BRIDGES (no callbacks/action2)
	&_nif_house,        // GSF_HOUSES
	NULL,               // GSF_GLOBALVAR (has no "physical" objects)
	&_nif_industrytile, // GSF_INDUSTRYTILES
	&_nif_industry,     // GSF_INDUSTRIES
	NULL,               // GSF_CARGOS (has no "physical" objects)
	NULL,               // GSF_SOUNDFX (has no "physical" objects)
	NULL,               // GSF_AIRPORTS (feature not implemented)
	NULL,               // GSF_SIGNALS (feature not implemented)
	NULL,               // GSF_OBJECTS (feature not implemented)
	&_nif_railtype,     // GSF_RAILTYPES
	&_nif_airporttile,  // GSF_AIRPORTTILES
	&_nif_town,         // GSF_FAKE_TOWNS
};
assert_compile(lengthof(_nifeatures) == GSF_FAKE_END);
