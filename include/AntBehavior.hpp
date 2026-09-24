#pragma once
#include "Ant.hpp"
#include "Cell.hpp"

// priority: 1 = highest ... 7 = lowest usable, 8 = Stay (never selected)
struct Decision { Action action = Action::Stay; int priority = 8; };
Decision evaluateTarget(const Ant& ant, const Cell& current, const Cell& target);
const char* actionName(Action action);
