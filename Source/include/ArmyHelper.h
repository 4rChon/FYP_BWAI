#pragma once
#include <BWAPI.h>
#include "MapHelper.h"

namespace ArmyHelper
{
	void initialiseHelper();
	void updateArmyMovement();
	void scout();
	void attack();
	void defend();
	void stopScouting();
	bool isDefending();	
	bool isAttacking();
	void addTargetPriority(BWAPI::Unit unit);
	void removeTargetPriority(BWAPI::Unit unit);
	void updateTargetPriority();
	void clearZoneTargets(MapHelper::Zone * zone);
	bool scan(BWAPI::Position target);
	void printPriorityList(int i = 0);
}