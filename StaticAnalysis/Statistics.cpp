#include "Statistics.hpp"

#include <Automation/AutomationModel.hpp>
#include <JS/JSProcessModel.hpp>
#include <Mapping/MappingModel.hpp>

#include <Interpolation/InterpolationProcess.hpp>

namespace stal
{
ScenarioStatistics::ScenarioStatistics(const Scenario::ProcessModel& scenar)
{
  intervals = scenar.intervals.size();
  events = scenar.events.size();
  nodes = scenar.timeSyncs.size();
  states = scenar.states.size();

  for (auto& itv : scenar.intervals)
  {
    if (itv.processes.empty())
    {
      empty_intervals++;
    }
    else
    {
      processes += itv.processes.size();
      for (auto& proc : itv.processes)
      {
        if (dynamic_cast<Automation::ProcessModel*>(&proc))
          automations++;
        else if (dynamic_cast<Mapping::ProcessModel*>(&proc))
          mappings++;
        else if (dynamic_cast<Scenario::ProcessModel*>(&proc))
          scenarios++;
        // else if (dynamic_cast<Interpolation::ProcessModel*>(&proc))
        //  interpolations++;
        else if (dynamic_cast<JS::ProcessModel*>(&proc))
          scripts++;
        else
          other++;
      }
    }
  }
  for (auto& st : scenar.states)
  {
    if (st.messages().rootNode().childCount() == 0)
      empty_states++;
  }
  for (auto& ev : scenar.events)
  {
    if (ev.condition().childCount() != 0
        && ev.condition() != State::Expression{})
      conditions++;
  }
  for (auto& node : scenar.timeSyncs)
  {
    if (node.expression().childCount() != 0
        && node.expression() != State::Expression{})
      triggers++;
  }

  if (intervals > 0 && intervals != empty_intervals)
  {
    processesPerInterval = double(processes) / double(intervals);
    processesPerIntervalWithProcess
        = double(processes) / double(intervals - empty_intervals);
  }
}

GlobalStatistics::GlobalStatistics(const Scenario::IntervalModel& itv)
{
  for (auto& proc : itv.processes)
  {
    if (dynamic_cast<Automation::ProcessModel*>(&proc))
      automations++;
    else if (dynamic_cast<Mapping::ProcessModel*>(&proc))
      mappings++;
    else if (auto scenar = dynamic_cast<Scenario::ProcessModel*>(&proc))
    {
      scenarios++;
      visit(*scenar);
    }
    // else if (dynamic_cast<Interpolation::ProcessModel*>(&proc))
    //   interpolations++;
    else if (dynamic_cast<JS::ProcessModel*>(&proc))
      scripts++;
    else
      other++;
  }
}

void GlobalStatistics::visit(const Scenario::ProcessModel& scenar)
{
  ScenarioStatistics st{scenar};
  intervals += st.intervals;
  empty_intervals += st.empty_intervals;
  events += st.events;
  nodes += st.nodes;
  empty_states += st.empty_states;
  states += st.states;

  conditions += st.conditions;
  triggers += st.triggers;

  processes += st.processes;

  automations += st.automations;
  interpolations += st.interpolations;
  mappings += st.mappings;
  scenarios += st.scenarios;
  scripts += st.scripts;
  other += st.other;

  for (auto& itv : scenar.intervals)
    visit(itv);
}

void GlobalStatistics::visit(const Scenario::IntervalModel& itv)
{
  curDepth++;
  if (curDepth > maxDepth)
    maxDepth = curDepth;
  for (auto& proc : itv.processes)
  {
    if (auto scenar = dynamic_cast<Scenario::ProcessModel*>(&proc))
    {
      visit(*scenar);
    }
  }
  curDepth--;
}

/*
void GlobalStatistics::visit(const Loop::ProcessModel& loop)
{
  states += 2;
  events += 2;
  nodes += 2;
  intervals += 1;
  if (loop.startState().messages().rootNode().childCount() == 0)
    empty_states++;
  if (loop.endState().messages().rootNode().childCount() == 0)
    empty_states++;

  if (loop.interval().processes.size() == 0)
    empty_intervals++;

  visit(loop.interval());
}
*/

}
