#include "entity.h"

inline entity*
GetEntity(game_state* gameState, u32 index)
{
    entity* entity = 0;
    if ((index > 0) && (index < ArrayCount(gameState->entities)))
    {
	entity = &gameState->entities[index];
    }
    return(entity);
}

internal u32
AddEntity(game_state* gameState)
{
    u32 entityIndex = gameState->entityCount++;
    Assert(gameState->entityCount < ArrayCount(gameState->entities));
    entity* result = &gameState->entities[entityIndex];
    return(entityIndex);
}

internal u32
AddEntity(game_state* gameState, entity* newEntity)
{
    u32 entityIndex = gameState->entityCount++;
    Assert(gameState->entityCount < ArrayCount(gameState->entities));
    gameState->entities[entityIndex] = *newEntity;
    return(entityIndex);
}

internal void
StatePoolFree(state_recorder* recording, void* ptr)
{
    if ((recording == 0) || (ptr == 0))
    {
	return;
    }

    state_recorder_node* freed = (state_recorder_node*)ptr;
    freed->next = recording->freeNodes;
    recording->freeNodes = freed;
}

internal void*
StatePoolAlloc(state_recorder* recording)
{
    state_recorder_node* result = 0;
    if ((recording == 0) || (recording->freeNodes == 0))
    {
	return(result);
    }
    
    result = recording->freeNodes;
    recording->freeNodes = recording->freeNodes->next;
    return(result);
}

internal void
RemoveStateRecording(state_recorder* recorderMemory, state_recorder_node** recorderList)
{
    if (recorderList == 0)
    {
	return;
    }
    state_recorder_node* temp = *recorderList;
    *recorderList = (*recorderList)->next;

    //Now free temp from the pool
    StatePoolFree(recorderMemory, temp);
}

internal void
AddStateRecording(state_recorder* recorderMemory, tile_map_position newP, state_recorder_node** recorderList)
{
    state_recorder_node* newNode = (state_recorder_node*)StatePoolAlloc(recorderMemory);
    newNode->p = newP;

    if (recorderList == 0)
    {
	*recorderList = newNode;
    }
    else
    {
	newNode->next = *recorderList;
	*recorderList = newNode;
    }
}

internal void
InitializeParty(game_state* gameState, u32 entityIndex, v2 startingLoc, bool32 shouldCamFollow, entity_bitmap* entityBitmap)
{
    entity* entity = GetEntity(gameState, entityIndex);
    entity->exists = true;
    entity->floatingMovement = false;
    entity->p.absTileX = (u32)startingLoc.x;
    entity->p.absTileY = (u32)startingLoc.y;
    entity->startingLocation = startingLoc;    
    entity->p.offset.x = 0.0f;
    entity->p.offset.y = 0.0f;
    entity->originalP = entity->p;
    entity->height = 0.5f;
    entity->width = 1.0f;
    entity->canJump = true;
    entity->bitmap = entityBitmap;


    //Push first location onto list
//    AddRecording(&gameState->stateRecorder, entity->p, &gameState->worldArena, gameState);
    
    if (shouldCamFollow)
    {
	gameState->cameraFollowingEntityIndex = entityIndex;
	AddStateRecording(gameState->stateRecorderMemory, entity->p, &gameState->stateRecorderList);	
    }
}


internal bool32
TestWall(r32 wallX, r32 relX, r32 relY, r32 playerDeltaX, r32 playerDeltaY, r32* tMin,
	 r32 minY, r32 maxY)
{
    bool32 hit = false;
    r32 epsilon = 0.0001f;

    if (playerDeltaX != 0.0f)
    {
	r32 tResult = (wallX - relX) / playerDeltaX;
	r32 y = relY + tResult * playerDeltaY;
	if ((tResult >= 0.0f) && (*tMin > tResult))
	{
	    if ((y >= minY) && (y <= maxY))
	    {
		*tMin = Maximum(0.0f, tResult - epsilon);
		hit = true;
	    }
	}
    }
    return(hit);
}

internal void
GetWallNormal(tile_map* tileMap, entity* entity, r32* tMin, entity_movement_calculations* entityInfo, tile_map_position testTileP, v2* wallNormal)
{
    r32 diameterW = tileMap->tileSideInMeters + entity->width;
    r32 diameterH = tileMap->tileSideInMeters + entity->height;
    v2 minCorner = -0.5f*v2{diameterW, diameterH};
    v2 maxCorner = 0.5f*v2{diameterW, diameterH};

    tile_map_difference relOldPlayerP = Subtract(tileMap, &entity->p, &testTileP);
    v2 rel = relOldPlayerP.dXY;
    v2 result = {};
    if (TestWall(minCorner.x, rel.x, rel.y, entityInfo->entityDelta.x, entityInfo->entityDelta.y,
		 tMin, minCorner.y, maxCorner.y))
    {
	*wallNormal = v2{-1, 0};
    }
    if (TestWall(maxCorner.x, rel.x, rel.y, entityInfo->entityDelta.x, entityInfo->entityDelta.y,
		 tMin, minCorner.y, maxCorner.y))
    {
	*wallNormal = v2{1, 0};
    }
    if (TestWall(minCorner.y, rel.y, rel.x, entityInfo->entityDelta.y, entityInfo->entityDelta.x,
		 tMin, minCorner.x, maxCorner.x))
    {
	*wallNormal = v2{0, -1};
    }
    if (TestWall(maxCorner.y, rel.y, rel.x, entityInfo->entityDelta.y, entityInfo->entityDelta.x,
		 tMin, minCorner.x, maxCorner.x))
    {
	*wallNormal = v2{0, 1};
    }
}

internal test_tile_dimensions
GetTestTileDimensions(tile_map_position oldP, tile_map_position newP, tile_map* tileMap, entity* entity)
{
    test_tile_dimensions result = {};
    result.minTileX = Minimum(oldP.absTileX, newP.absTileX);
    result.minTileY = Minimum(oldP.absTileY, newP.absTileY);
    result.maxTileX = Maximum(oldP.absTileX, newP.absTileX);
    result.maxTileY = Maximum(oldP.absTileY, newP.absTileY);

    u32 entityTileWidth = CeilReal32ToInt32(entity->width / tileMap->tileSideInMeters);
    u32 entityTileHeight = CeilReal32ToInt32(entity->height / tileMap->tileSideInMeters);

    result.minTileX -= entityTileWidth;
    result.minTileY -= entityTileHeight;
    result.maxTileX += entityTileWidth;
    result.maxTileY += entityTileHeight;

    return(result);
}

internal entity_movement_calculations
CalculateNewP(tile_map* tileMap, v2 ddP, entity* entity, r32 dt)
{

    entity_movement_calculations result = {};
    r32 ddPLength = LengthSq(ddP);
    if (ddPLength > 1.0f)
    {
	ddP *= 1.0f / SquareRoot(ddPLength);
    }

    //playerSpeed must be determined by the distance of the mouse cursor
    ddP *= entity->entitySpeed;
    
    ddP += -1.0f*entity->dP;


    result.oldP = entity->p;

    result.entityDelta = (0.5f * ddP * Square(dt) + entity->dP*dt);

    r32 gravity = 0.0f;
    entity->dP = ddP * dt + entity->dP;


    result.newP = Offset(tileMap, result.oldP, result.entityDelta);
    return(result);
}

//This is kind of a magic function but it is what it is
internal void
EntityCollisionRoutine(test_tile_dimensions dim, entity* entity, tile_map* tileMap, entity_movement_calculations* entityInfo, bool32 isFloor, r32* tMin, v2* wallNormal)
{
    Assert((dim.maxTileX - dim.minTileX) < 32);
    Assert((dim.maxTileY - dim.minTileY) < 32);

    
    u32 absTileZ = entity->p.absTileZ;

    for (u32 absTileY = dim.minTileY; absTileY <= dim.maxTileY; ++absTileY)
    {
	for (u32 absTileX = dim.minTileX; absTileX <= dim.maxTileX; ++absTileX)
	{
	    tile_map_position testTileP = CenteredTilePoint(absTileX, absTileY, absTileZ);
	    tile_value tileValue = GetTileValue(tileMap, testTileP);

	    if (IsTileValueEmpty(tileValue) != e_collision_type::none)
	    {
		switch (tileValue.collisionType)
		{
		case e_collision_type::block:
		{
		    //Didn't actually end up needing to compact this but oh well
		    GetWallNormal(tileMap, entity, tMin, entityInfo, testTileP, wallNormal);
		} break;
		case e_collision_type::response:
		{
		    if (tileValue.collisionResponse != e_collision_response::noResponse)
		    {
			switch (tileValue.collisionResponse)
			{
			case e_collision_response::goalResponse:
			{
			    if ((ball_entity*)entity)
			    {
				//Player hit the goal, now what

				if ((testTileP.absTileX == entity->p.absTileX) &&
				    (testTileP.absTileY == entity->p.absTileY))
				{
				    Assert("Congrats ball was hit properly");
				}
			    }
			} break;
			case e_collision_response::noResponse:
			{
			    Assert("Should not be hitting this");
			} break;
			}
		    }
		} break;
		case e_collision_type::none:
		{
		    Assert("Uhhh");
		} break;
		default:
		{
		    Assert("Defaulted here");
		} break;
		}
	    }
	    else
	    {

	    }
	}
    }    
}

internal void
MoveBall(game_state* gameState, entity* entity, r32 dt, v2 ddP)
{
    tile_map* tileMap = gameState->world->tileMap;
    entity_movement_calculations ballInfo = CalculateNewP(tileMap, ddP, entity, dt);

    test_tile_dimensions dim = GetTestTileDimensions(ballInfo.oldP, ballInfo.newP, tileMap, entity);

    r32 tRemaining = 1.0f;
    r32 tMin = 1.0f;

    bool32 isFloor = false;

    v2 vReflection = {};
    for(u32 iteration = 0; (iteration < 4) && (tRemaining > 0.0f); ++iteration)
    {
	tMin = 1.0f;
	v2 wallNormal = {};

	EntityCollisionRoutine(dim, entity, tileMap, &ballInfo, isFloor, &tMin, &wallNormal);

	entity->p = Offset(tileMap, entity->p, tMin*ballInfo.entityDelta);
	vReflection = ReflectionVector(entity->dP, wallNormal);
	
	entity->dP = vReflection;

	ballInfo.entityDelta = ballInfo.entityDelta - 1 * Inner(ballInfo.entityDelta, wallNormal)*wallNormal;
	tRemaining -= tMin;
    }
}    

internal void
MovePlayer(v2 ddP, entity* entity, game_state* gameState, bool32 newStep)
{
    tile_map* tileMap = gameState->world->tileMap;
    //Use tile_map_position which has absTile... for locations

    tile_map_position projectedLocation = entity->p;

    tile_map_position oldPosition = projectedLocation;

    if (newStep)
    {
	projectedLocation.absTileZ = 0;
	if (ddP.x == 1.0f)
	{
	    projectedLocation.absTileX++;
	}
	else if (ddP.x == -1.0f)
	{
	    projectedLocation.absTileX--;
	}
	else if (ddP.y == 1.0f)
	{
	    projectedLocation.absTileY++;
	}
	else if (ddP.y == -1.0f)
	{
	    projectedLocation.absTileY--;
	}
    }
    else
    {
	//get a recorded input and remove it from the list
	if (gameState->stateRecorderList)
	{
	    projectedLocation = gameState->stateRecorderList->p;
	    RemoveStateRecording(gameState->stateRecorderMemory, &gameState->stateRecorderList);
	}
    }
    tile_map_position testTileP = CenteredTilePoint(projectedLocation.absTileX, projectedLocation.absTileY, projectedLocation.absTileZ);
    tile_value tileValue = GetTileValue(tileMap, testTileP);


    if (CanPlayerWalkInTile(tileValue))
    {
	entity->p = projectedLocation;	
    }


    bool32 hasMoved = (oldPosition.absTileX != projectedLocation.absTileX ||
		       oldPosition.absTileY != projectedLocation.absTileY);
    //Add new recording to the inputs
    if (newStep && hasMoved)
    {
	AddStateRecording(gameState->stateRecorderMemory, oldPosition, &gameState->stateRecorderList);
    }
}

///
internal void
MovePlayer(game_state* gameState, entity* entity, r32 dt, v2 ddP)
{
    tile_map* tileMap = gameState->world->tileMap;

    entity_movement_calculations playerInfo = CalculateNewP(tileMap, ddP, entity, dt);


    test_tile_dimensions dim = GetTestTileDimensions(playerInfo.oldP, playerInfo.newP, tileMap, entity);


    r32 tRemaining = 1.0f;
    r32 tMin = 1.0f;

    bool32 isFloor = false;
    

    for (u32 iteration = 0; (iteration < 4) && (tRemaining > 0.0f); ++iteration)
    {
	tMin = 1.0f;
	v2 wallNormal = {};

	EntityCollisionRoutine(dim, entity, tileMap, &playerInfo, isFloor, &tMin, &wallNormal);

	entity->p = Offset(tileMap, entity->p, tMin*playerInfo.entityDelta);
	entity->dP = entity->dP - 1*Inner(entity->dP, wallNormal)*wallNormal;
	//I need to be able to edit the bounciness value as well as the way the object bounces
	playerInfo.entityDelta = playerInfo.entityDelta - 1 * Inner(playerInfo.entityDelta, wallNormal)*wallNormal;
	tRemaining -= tMin;
    }


    //I don't need this for the rest of the entity types
    if ((entity->dP.x == 0) && (entity->dP.y == 0))
    {
	
    }
    else if (AbsoluteValue(entity->dP.x) > AbsoluteValue(entity->dP.y))
    {
	if (entity->dP.x > 0)
	{
	    entity->facingDirection = 0;
	}
	else
	{
	    entity->facingDirection = 2;
	}
    }
    else
    {
	if (entity->dP.x > 0)
	{
	    entity->facingDirection = 1;
	}
	else
	{
	    entity->facingDirection = 3;
	}
    }
}

