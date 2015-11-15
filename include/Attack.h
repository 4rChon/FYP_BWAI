#pragma once
#include "Task.h"
#include "Zone.h"

class Attack : public Task
{
private:
	Zone* target;
	bool scouting;
public:
	//constructors and destructors
	Attack(Zone* target);

	//-
	void assign();
	void act();
	void update();
};