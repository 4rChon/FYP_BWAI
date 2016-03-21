#include "CreateUnit.h"
#include "CreateCoalition.h"
#include "EconHelper.h"
#include "SatisfyUnitRequirement.h"
#include "TaskHelper.h"
#include "CoalitionHelper.h"
#include "DesireHelper.h"
#include "EconHelper.h"
#include <string>

CreateUnit::CreateUnit(BWAPI::UnitType unitType, int unitCount)
{
	taskName = "CreateUnit(" + unitType.getName() + " - " + std::to_string(unitCount) + ")";
		
	this->unitType = unitType;	
	this->unitCount = unitCount;
	satisfying = false;	
	satisfied = true;
	requiresGas = false;
	taskType = CRU;

	//debug = true;
}

BWAPI::UnitType CreateUnit::getUnitType()
{
	return unitType;
}

void CreateUnit::satisfyRequirements()
{
	if (unitType.gasPrice() > 0 && EconHelper::getGasIncome() == 0)
	{
		requiresGas = true;
		satisfied = false;		
	}

	if(!BWAPI::Broodwar->canMake(unitType))
		satisfied = false;

	if (!satisfying && !satisfied)
	{
		printDebugInfo("Attempting to Satisfy Unit - Unit Requirement");
		SatisfyUnitRequirement* satisfyUnitRequirement = new SatisfyUnitRequirement(unitType);
		addSubTask(satisfyUnitRequirement);
		if (requiresGas)
		{
			CreateUnit* createGas = new CreateUnit(BWAPI::Broodwar->self()->getRace().getRefinery());
			addSubTask(createGas);
		}
		satisfying = true;
	}
}

void CreateUnit::createCoalition()
{
	printDebugInfo("Unit requirements satisfied");
	Composition producer;
	auto whatBuilds = unitType.whatBuilds().first;

	
	if (!whatBuilds.isWorker())
	{
		for (int i = 0; i < unitCount / 5; i++)
			producer.addType(whatBuilds, unitType.whatBuilds().second);
	}
	else
		producer.addType(whatBuilds, unitType.whatBuilds().second);
	CreateCoalition* createCoalition = new CreateCoalition(producer, this);
	subTasks.insert(createCoalition);
}

void CreateUnit::decrementUnitCount()
{
	printDebugInfo("Decrementing Unit Count");
	unitCount--;
}

// assign a producer coalition
void CreateUnit::assign()
{
	printDebugInfo("Assign");
	satisfied = true;	
	satisfyRequirements();
		
	if (satisfied && EconHelper::haveMoney(unitType) && EconHelper::haveSupply(unitType))
	{
		createCoalition();
		assigned = true;
		printDebugInfo("Assigned!");
	}
	printDebugInfo("Assign End");
}

// produce a unit
void CreateUnit::act()
{	
	printDebugInfo("Acting");	
	if (unitCount > 0)
	{
		if (!EconHelper::haveMoney(unitType))
			return;
		printDebugInfo("UnitCount: " + std::to_string(unitCount));
		if (unitType == BWAPI::Broodwar->self()->getRace().getRefinery())
		{
			for each (auto &agent in coalition->getAgentSet())
			{
				for each (auto &resourceDepot in AgentHelper::getResourceDepots())
					if (resourceDepot->addGeyser((Worker*)agent))
						break;
			}
			return;
		}

		if (unitType == BWAPI::UnitTypes::Zerg_Lurker || unitType == BWAPI::UnitTypes::Zerg_Guardian)
		{
			for each (auto &agent in coalition->getAgentSet())
				if (agent->morph(unitType))
					unitCount--;
			return;
		}

		if (unitType == BWAPI::UnitTypes::Protoss_Archon)
		{
			if(coalition->getAgentSet().size() % 2 == 0)
				for (auto agent = coalition->getAgentSet().begin(); agent != coalition->getAgentSet().end(); ++agent)
				{
					if ((*agent)->useAbility(BWAPI::TechTypes::Archon_Warp, (*++agent)->getUnit()))
						unitCount--;
				}
			return;
		}

		if (unitType == BWAPI::UnitTypes::Protoss_Dark_Archon)
		{
			if (coalition->getAgentSet().size() % 2 == 0)
				for (auto &agent = coalition->getAgentSet().begin(); agent != coalition->getAgentSet().end(); ++agent)
				{
					if ((*agent)->useAbility(BWAPI::TechTypes::Dark_Archon_Meld, (*++agent)->getUnit()))
						unitCount--;
				}
			return;
		}

		if (unitType.isAddon())
		{
			for each (auto &agent in coalition->getAgentSet())
			{
				if(agent->buildAddon(unitType))
					unitCount--;
			}
			return;
		}

		if (unitType.isBuilding())
		{
			for each (auto &agent in coalition->getAgentSet())
			{
				if (unitType.getRace() == BWAPI::Races::Zerg
					&& (unitType.isResourceDepot() && unitType != BWAPI::UnitTypes::Zerg_Hatchery)
					|| unitType == BWAPI::UnitTypes::Zerg_Greater_Spire)
				{
					if (agent->morph(unitType))
						unitCount--;
				}
				else
					agent->build(unitType);
						//unitCount--;
			}
			return;
		}
		else
		{
			for each (auto &agent in coalition->getAgentSet())
				if (agent->train(unitType))
					unitCount--;
			return;
		}
	}
	else
		succeed();
	printDebugInfo("Acting End");
}

void CreateUnit::update() //x2 redundant in res tech
{
	printDebugInfo("Update");
	if (complete)
		return;

	if (unitType.isRefinery() && BWAPI::Broodwar->self()->allUnitCount(BWAPI::Broodwar->self()->getRace().getRefinery()) >= AgentHelper::getResourceDepots().size())
	{
		succeed();
		return;
	}

	if (!EconHelper::haveSupply(unitType) && BWAPI::Broodwar->self()->supplyTotal() >= 400)
	{
		succeed();
		return;
	}

	if (!assigned)
	{
		assign();
		return;
	}

	if (coalition->isActive())
	{
		act();		
	}
	printDebugInfo("Update End");
}