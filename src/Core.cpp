#include "Core.h"
#include "Worker.h"
#include "Producer.h"
#include "AgentManager.h"
#include "CoalitionManager.h"
#include "Scout.h"

using namespace BWAPI;
using namespace Filter;

bool scouting = false;

void updateTaskTree(Task* task)
{
	if (task->getSubTasks().size() > 0)
		for (auto t : task->getSubTasks())
			updateTaskTree(t);
	task->assign();
	task->update();
}

void initSatisifed()
{
	std::cout << "Initialising Satisfied Types\n";
	for each(BWAPI::UnitType t in BWAPI::UnitTypes::allUnitTypes())
		g_isUnlocked[t] = false;
}

void updateSatisfied()
{
	for (auto unit : g_TotalCount)
		if (unit.second > 0)
			for (auto buildsThis : unit.first.buildsWhat())
				for (auto requiresThis : buildsThis.requiredUnits())
				{
					if (g_TotalCount[requiresThis.first] == 0)
					{
						g_isUnlocked[buildsThis] = false;
						break;
					}
					g_isUnlocked[buildsThis] = true;
				}

	if (Broodwar->self()->getRace() == Races::Zerg)
		g_isUnlocked[UnitTypes::Zerg_Larva] = true;

	AgentManager::getInstance()->updateUnlocked();
}

void Core::onStart()
{
	std::cout << "\n------------ MATCH STARTED --------------\n";

	initSatisifed();
	updateSatisfied();

	AgentManager::getInstance();
	CoalitionManager::getInstance();
	ThreatField::getInstance();	

	drawGui = false;

	defend = new Defend(ThreatField::getInstance()->getZone(140));
	attack = nullptr;
	scout = nullptr;

	Broodwar->sendText("gl hf");
	Broodwar << "The map is " << Broodwar->mapName() << "!" << std::endl;


	Broodwar->enableFlag(Flag::UserInput);

	// Uncomment the following line and the bot will know about everything through the fog of war (cheat).
	//Broodwar->enableFlag(Flag::CompleteMapInformation);

	Broodwar->setCommandOptimizationLevel(1);

	if (Broodwar->isReplay())
	{
		Broodwar << "The following players are in this replay:" << std::endl;

		Playerset players = Broodwar->getPlayers();
		for (auto p : players)
			if (!p->isObserver())
				Broodwar << p->getName() << ", playing as " << p->getRace() << std::endl;

	}
	else
		if (Broodwar->enemy())
			Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;

}

void Core::onEnd(bool isWinner)
{
	delete attack;
	if (isWinner)
	{
	}

	std::cout << "\n------------  MATCH ENDED  --------------\n\n";

}

void Core::onFrame()
{
	//display text with information at the top of the screen
	drawTextInfo();

	if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self())
		return;

	if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
		return;

	if (drawGui)
		drawRegions();

	std::unordered_set<Agent*>::iterator agent = AgentManager::getInstance()->getAgentset().begin();
	while (agent != AgentManager::getInstance()->getAgentset().end())
	{
		if (!(*agent)->getUnit()->exists())
		{
			agent = AgentManager::getInstance()->removeAgent(agent);
			continue;
		}

		if ((*agent)->getUnit()->isLockedDown() || (*agent)->getUnit()->isMaelstrommed() || (*agent)->getUnit()->isStasised())
		{
			++agent;
			continue;
		}

		if ((*agent)->getUnit()->isLoaded() || !(*agent)->getUnit()->isPowered() || (*agent)->getUnit()->isStuck())
		{
			++agent;
			continue;
		}

		if (!(*agent)->getUnit()->isCompleted() || (*agent)->getUnit()->isConstructing())
		{
			++agent;
			continue;
		}

		if ((*agent)->isFree())
		{
			auto coalition = CoalitionManager::getInstance()->getCoalitionset().begin();
			while (coalition != CoalitionManager::getInstance()->getCoalitionset().end())
			{
				if (!(*coalition)->isActive())
					(*coalition)->addAgent((*agent)); //probability parameter

				++coalition;
			}
			(*agent)->act();
		}

		ThreatField::getInstance()->getZone((*agent)->getUnit()->getRegion()->getID())->updateZone();

		++agent;
	}

	if (attack != nullptr)
	{
		if (attack->isComplete())
		{
			delete attack;
			attack = nullptr;
			defend = new Defend(ThreatField::getInstance()->getZone(135));
		}
		else
			updateTaskTree(attack);
	}

	
	if (defend != nullptr)
	{
		if (defend->isComplete())
		{
			delete defend;
			defend = nullptr;			
			if (g_attackTarget >= 0)
			{
				std::cout << "Attack Task: " << g_attackTarget << "\n";
				attack = new Attack(ThreatField::getInstance()->getZone(g_attackTarget));
			}
		}
		else updateTaskTree(defend);
	}

	if (defend == nullptr && attack == nullptr && scout == nullptr)
	{
		if (g_attackTarget >= 0)
		{
			std::cout << "Attack Task: " << g_attackTarget << "\n";
			attack = new Attack(ThreatField::getInstance()->getZone(g_attackTarget));
		}
		else
			scout = new Scout(ThreatField::getInstance()->getRandomZone());
	}

	if (scout != nullptr)
	{		
		if (scout->isComplete())
		{
			delete scout;
			scout = nullptr;
		}
		else updateTaskTree(scout);
	}
}

void Core::onSendText(std::string text)
{
	if (text == "/draw gui")
	{
		drawGui = !drawGui;
		return;
	}

	Broodwar->sendText("%s", text.c_str());
}

void Core::onReceiveText(BWAPI::Player player, std::string text)
{
	Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void Core::onPlayerLeft(BWAPI::Player player)
{
	Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void Core::onNukeDetect(BWAPI::Position target)
{
	if (target)
		Broodwar << "Nuclear Launch Detected at " << target << std::endl;
	else
		Broodwar->sendText("Where's the nuke?");

}

void Core::onUnitDiscover(BWAPI::Unit unit)
{
}

void Core::onUnitEvade(BWAPI::Unit unit)
{	
}

void Core::onUnitShow(BWAPI::Unit unit)
{
	if (unit->getType().isResourceDepot() && unit->getPlayer() != Broodwar->self())
	{
		std::cout << "Hey i found the town center\n";
		g_attackTarget = unit->getRegion()->getID();
	}
}

void Core::onUnitHide(BWAPI::Unit unit)
{
}

void Core::onUnitCreate(BWAPI::Unit unit)
{
}

void Core::onUnitDestroy(BWAPI::Unit unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		g_TotalCount[unit->getType()]--;
		AgentManager::getInstance()->removeAgent(unit);
	}
	updateSatisfied();
}

void Core::onUnitMorph(BWAPI::Unit unit)
{
	if (Broodwar->isReplay())
		if (unit->getType().isBuilding() && !unit->getPlayer()->isNeutral())
		{
			int seconds = Broodwar->getFrameCount() / 24;
			int minutes = seconds / 60;
			seconds %= 60;
			Broodwar->sendText("%.2d:%.2d: %s morphs a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
		}
}

void Core::onUnitRenegade(BWAPI::Unit unit)
{
}

void Core::onSaveGame(std::string gameName)
{
	Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void Core::onUnitComplete(BWAPI::Unit unit)
{
	if (unit->getPlayer() == Broodwar->self())
	{
		AgentManager::getInstance()->addAgent(unit);
		g_TotalCount[unit->getType()]++;
	}
	updateSatisfied();
	g_Supply = (1 / (double)((BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed()) + 1));
}

void Core::drawRegions()
{
	for (int i = 0; i < ThreatField::getInstance()->getSize(); i++)
	{
		Color c = Color(255, 0, 0);
		if (ThreatField::getInstance()->getZone(i)->getRegion()->getUnits(Filter::IsOwned).size() > 0)
			c = Color(0, 255, 0);
		Broodwar->drawCircle(CoordinateType::Map, ThreatField::getInstance()->getZone(i)->getRegion()->getCenter().x, ThreatField::getInstance()->getZone(i)->getRegion()->getCenter().y, 2, c, true);

		std::ostringstream oss;
		oss << "getRegion() ID: " << ThreatField::getInstance()->getZone(i)->getRegion()->getID()
			<< "\nZone ID: " << i
			<< "\nEnemy Score: " << ThreatField::getInstance()->getZone(i)->getEnemyScore()
			<< "\nFriend Score: " << ThreatField::getInstance()->getZone(i)->getFriendScore()
			<< "\nResource Score: " << ThreatField::getInstance()->getZone(i)->getResourceScore()
			//<< "\nConfidence Value " << threatField->getZone(i)->getConfidence()
			<< "\nLast Visit " << ThreatField::getInstance()->getZone(i)->getLastVisit()
			<< "\nPosition: " << "(" << ThreatField::getInstance()->getZone(i)->getRegion()->getCenter().x << ", " << ThreatField::getInstance()->getZone(i)->getRegion()->getCenter().y << ")"
			<< "\nDefense Priority: " << ThreatField::getInstance()->getZone(i)->getRegion()->getDefensePriority()
			<< std::endl;
		std::string s = oss.str();

		Broodwar->drawTextMap(
			ThreatField::getInstance()->getZone(i)->getRegion()->getCenter().x, ThreatField::getInstance()->getZone(i)->getRegion()->getCenter().y,
			s.c_str()
			);
	}
}

void Core::drawTextInfo()
{
	Broodwar->drawTextScreen(200, 0, "FPS: %d", Broodwar->getFPS());
	Broodwar->drawTextScreen(200, 10, "Average FPS: %f", Broodwar->getAverageFPS());
	Broodwar->drawTextScreen(200, 30, "Active Tasks: %d", g_Tasks.size());
	Broodwar->drawTextScreen(200, 40, "Coalition Count: %d", CoalitionManager::getInstance()->getCoalitionset().size());
	Broodwar->drawTextScreen(200, 50, "Open Coalition Count: %d", CoalitionManager::getInstance()->getOpenCount());
	Broodwar->drawTextScreen(200, 60, "Agent Count: %d", AgentManager::getInstance()->getAgentset().size());
	Broodwar->drawTextScreen(200, 70, "Free Agent Count: %d", AgentManager::getInstance()->getFreeCount());
	Broodwar->drawTextScreen(200, 80, "Supply Desire: %.5f", g_Supply);
	Broodwar->drawTextScreen(200, 90, "Mineral Reserve: %.5f", g_MinReserve);
	Broodwar->drawTextScreen(200, 100, "Gas Reserve: %.5f", g_GasReserve);

	for (auto coalition : CoalitionManager::getInstance()->getCoalitionset())
		if (coalition->isActive())
			for (auto unit : coalition->getUnitSet())
			{
				if (!unit->exists())
					coalition->removeUnit(unit);

				Broodwar->drawTextMap(unit->getPosition(), coalition->getCurrentTaskString().c_str());
			}
}