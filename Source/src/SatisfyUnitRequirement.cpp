#include "SatisfyUnitRequirement.h"
#include "CreateUnit.h"
#include "ResearchTech.h"
#include "TaskHelper.h"
#include "CoalitionHelper.h"

SatisfyUnitRequirement::SatisfyUnitRequirement(BWAPI::UnitType unitType)
{
	taskName = "SatisfyUnitRequirement(" + unitType.getName() + ")";
	
	this->unitType = unitType;
	taskType = SUR;
	//debug = true;
}

void SatisfyUnitRequirement::assign()
{
	printDebugInfo("Assign");
	assigned = true;
	printDebugInfo("Assign End");
}

void SatisfyUnitRequirement::act()
{
	//create units to satisfy requirements
	printDebugInfo("Acting");
	for (auto &requirement : unitType.requiredUnits())
	{
		printDebugInfo("Require : " + requirement.first.getName());
		if (!BWAPI::Broodwar->self()->hasUnitTypeRequirement(requirement.first, requirement.second)
			&& BWAPI::Broodwar->self()->incompleteUnitCount(requirement.first) < requirement.second)
		{
			CreateUnit* createUnit = new CreateUnit(requirement.first, requirement.second - AgentHelper::getTypeCount(requirement.first));
			printDebugInfo(" Creating : " + requirement.first.getName() + " : " + std::to_string(requirement.second));
			addSubTask(createUnit);
		}
	}

	if(unitType.requiredTech() != BWAPI::TechTypes::None)
	{
		ResearchTech* researchTech = new ResearchTech(unitType.requiredTech());
		addSubTask(researchTech);
	}

	acting = true;
	printDebugInfo("Acting End");
}

void SatisfyUnitRequirement::update()
{
	printDebugInfo("Update");
	if (complete)
		return;

	if (!assigned)
	{
		assign();
		return;
	}

	if (!acting && assigned)
	{
		act();
		return;
	}

	if (acting)
	{
		for each (auto &requirement in unitType.requiredUnits())
		{
			if (!BWAPI::Broodwar->self()->hasUnitTypeRequirement(requirement.first, requirement.second))
				return;
		}
		if (unitType.requiredTech() != BWAPI::TechTypes::None)
		{
			if (!BWAPI::Broodwar->self()->hasResearched(unitType.requiredTech()))
				return;
		}
		succeed();
	}
	printDebugInfo("Update End");
}
