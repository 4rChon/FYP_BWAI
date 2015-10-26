#include "CreateCoalition.h"
#include "CreateUnit.h"
#include "Globals.h"

CreateCoalition::CreateCoalition(Composition composition, Task* task)
{	
	this->taskType = CRC;
	this->taskName = "CreateCoalition(Composition, Task*)";
	this->taskCoalition = new Coalition(composition, task->getType());
	task->setCoalition(this->taskCoalition);
	g_Coalitions.insert(taskCoalition);
	g_OpenCoalitions.insert(taskCoalition);
}

void CreateCoalition::assign()
{
	if (!this->assigned)
	{
		if (!taskCoalition->isActive())
			for (auto unitType : taskCoalition->getTargetComp().getTypes())
				if (taskCoalition->getCurrentComp()[unitType] < taskCoalition->getTargetComp()[unitType])
				{
					std::cout << "Coalition Requires more " << unitType.c_str() << "\n";
					CreateUnit* createUnit = new CreateUnit(unitType);
					this->subTasks.push_back(createUnit);
				}
		this->assigned = true;
	}
}

void CreateCoalition::act()
{
	for (auto unitType : taskCoalition->getTargetComp().getTypes())
	{
		if (taskCoalition->getCurrentComp()[unitType] < taskCoalition->getTargetComp()[unitType])
		{
			//std::cout << "Coalition Requires more " << unitType.c_str() << "\n";			
			//CreateUnit* createUnit = new CreateUnit(unitType);
			//this->subTasks.push_back(createUnit);
		}
	}
}

void CreateCoalition::update()
{
	if (this->assigned && this->taskCoalition->isActive())
	{		
		cleanSubTasks(this->subTasks);		

		this->complete = true;
		coalition->disband();

		g_Tasks.remove(this);		

		return;
	}

	act();
}