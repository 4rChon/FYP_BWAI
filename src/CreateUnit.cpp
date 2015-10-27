#include "CreateUnit.h"
#include "CreateCoalition.h"
#include "Globals.h"
#include "SatisfyRequirement.h"

CreateUnit::CreateUnit(BWAPI::UnitType unitType)
{
	this->taskType = CRU;
	this->taskName = "CreateUnit(unitType)";
	this->unitType = unitType;
}

void CreateUnit::assign()
{
	if (!assigned)
	{
		std::cout << "Assigning Create " << unitType.c_str() << "\n";
		if (!g_isUnlocked[unitType])
		{
			std::cout << unitType.c_str() << " unavailable\n";
			SatisfyRequirement* satisfyRequirement = new SatisfyRequirement(unitType);
			subTasks.push_back(satisfyRequirement);
		}

		if (unitType.supplyRequired() > BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed())
		{
			std::cout << "You must construct additional pylons\n";
			CreateUnit* createUnit = new CreateUnit(BWAPI::Broodwar->self()->getRace().getSupplyProvider());
			subTasks.push_back(createUnit);
		}

		Composition producer;
		producer.addType(unitType.whatBuilds().first, unitType.whatBuilds().second);
		std::cout << "Composition created: \n";
		for (auto unit : producer.getUnitMap())
			std::cout << "\t" << unit.first.c_str() << " : " << unit.second << "\n";
		
		CreateCoalition* createCoalition = new CreateCoalition(producer, this);
		subTasks.push_back(createCoalition);
		assigned = true;
	}
}

void CreateUnit::act()
{
	if (!this->acting && this->assigned)
	{

		if (unitType.mineralPrice() > BWAPI::Broodwar->self()->minerals())
		{
			//CreateUnit* createUnit = new CreateUnit(BWAPI::Broodwar->self()->getRace().getWorker());
			//subTasks.push_back(createUnit);
			return;
		}
		std::cout << "Creating Unit\n";
		if (unitType.whatBuilds().first.isBuilding())
		{
			this->coalition->getUnitSet().train(unitType);
			this->acting = true;
			this->complete = true;
		}
		else if (unitType.isBuilding())
		{
			for (auto builder : this->coalition->getUnitSet())
			{
				BWAPI::TilePosition targetBuildLocation = BWAPI::Broodwar->getBuildLocation(unitType, builder->getTilePosition());
				if (targetBuildLocation)
				{
					if (builder->build(unitType, targetBuildLocation))
					{
						this->acting = true;
						this->complete = true;
					}
					// Register an event that draws the target build location
					auto ut = unitType;
					BWAPI::Broodwar->registerEvent([targetBuildLocation, ut](BWAPI::Game*)
					{
						BWAPI::Broodwar->drawBoxMap(BWAPI::Position(targetBuildLocation),
							BWAPI::Position(targetBuildLocation + ut.tileSize()),
							BWAPI::Colors::Blue);
					},
						nullptr,  // condition
						ut.buildTime() + 100);  // frames to run
				}
			}
		}
	}		
}

void CreateUnit::update()
{
	if (this->coalition->isActive() && !this->complete)
	{
		act();

		cleanSubTasks();		

		g_Tasks.remove(this);
	}

	if (this->complete)
		coalition->disband();
}