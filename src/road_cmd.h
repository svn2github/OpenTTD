/* $Id$ */

/** @file road_cmd.h Road related functions. */

#ifndef ROAD_CMD_H
#define ROAD_CMD_H

#include "direction_type.h"

void DrawRoadDepotSprite(int x, int y, DiagDirection dir, RoadType rt);
void InvalidateTownForRoadTile();

#endif /* ROAD_CMD_H */
