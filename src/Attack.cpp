#include "Attack.h"
#include "Composition.h"
#include "GlobalSets.h"
#include "CreateCoalition.h"

Attack::Attack(Zone* target)
{	
	this->taskType = ATT;
	this->taskName = "Attack(Zone*)";
	this->target = target;
}

Attack::~Attack()
{
	std::cout << "Attack Destructor\n";

	delete this->target;		
	this->target = nullptr;
}

// assign an attacking coalition
void Attack::assign()
{
	if (!this->assigned)
	{
		std::cout << "Attack: Assign\n";
		Composition c;
		c.addType(BWAPI::UnitTypes::Terran_Marine, 10);
		//c.addType(BWAPI::Broodwar->self()->getRace().getWorker(), 10);
		CreateCoalition *createCoalition = new CreateCoalition(c, this);
		addSubTask(createCoalition);
		this->assigned = true;
	}
}

// attack with coalition
void Attack::act()
{
	if (!this->acting)
	{
		std::cout << "Attack: Act\n";
		this->coalition->getUnitSet().attack(this->target->getRegion()->getCenter());
		this->acting = true;
	}
}

void Attack::update()
{
	if (this->complete)
	{
		this->cleanSubTasks();
		return;
	}
	
	if (this->assigned && this->coalition->isActive())
		act();
	if (this->target->getConfidence() > 0.8 && this->target->getEnemyScore() == 0)
	{		
		this->complete = true;		
		std::cout << "Attack: Complete\n";
	}	
}