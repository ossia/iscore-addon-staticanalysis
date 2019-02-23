#pragma once
#include <score/command/Dispatchers/CommandDispatcher.hpp>

namespace Scenario
{
class ProcessModel;
}
namespace stal
{
void generateScenario(
    const Scenario::ProcessModel& scenar,
    int N,
    CommandDispatcher<>&);

void generateScenarioFromPetriNet(
    const Scenario::ProcessModel& scenario,
    CommandDispatcher<>& disp);
}
