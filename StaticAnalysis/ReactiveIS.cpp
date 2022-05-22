#include "ReactiveIS.hpp"
#include "TAConversion.hpp"

#include <Automation/AutomationModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Scenario/Process/Algorithms/ContainersAccessors.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Process/ScenarioProcessMetadata.hpp>
namespace stal
{
using namespace TA;
/*
 *
structure : scenario {
    name : "This is the global scenario" ;
    start-when : True ;
    stop-when : True ;
    content : [
      texture : a {
          #Turning on/off lights#
          name : "Texture A" ;
          start-when : wait(start(scenario),1,1);
          stop-when : wait(start(a),2,2);
          msg-start : "/Light/1 on" ;
          msg-stop : "/Light/1 off" ;
      }
      ];
}


*/


struct ISVisitor
{
  std::string ss;
  int depth = 0;

  void append(std::string l)
  {
    ss += space();
    ss += l;
    ss += '\n';
  }


  void prop(std::string k, std::string v)
  {
    append(k + " : " + v + " ;");
  }

  auto object(std::string type, std::string name)
  {
    append(type + " : " + name + "{");
    depth ++;
    struct X {
      ISVisitor& self;
      ~X() {
        self.depth--;
        self.append("}");
      }
    };
    return X{*this};
  }

  std::string space() const;


  void visitProcesses(const Scenario::IntervalModel& c)
  {
    for (const auto& process : c.processes)
    {
      if (auto scenario = dynamic_cast<const Scenario::ProcessModel*>(&process))
      {
        this->visit(*scenario);
      }
      else if (auto autom = dynamic_cast<const Automation::ProcessModel*>(&process))
      {
        this->visit(*autom);
      }
    }
  }
  void visit(const Scenario::ScenarioInterface& scenario, const Scenario::TimeSyncModel& timenode);
  void visit(const Scenario::ScenarioInterface& scenario, const Scenario::EventModel& event);
  void visit(const Scenario::ScenarioInterface& scenario, const Scenario::IntervalModel& c);
  void visit(const Scenario::ScenarioInterface& scenario, const Scenario::StateModel& state);
  void visit(const Scenario::ScenarioInterface& s);
  void visit(const Automation::ProcessModel& s);
};





std::string ISVisitor::space() const
{
  return std::string(depth, ' ');
}

void ISVisitor::visit(const Scenario::ScenarioInterface& scenario, const Scenario::TimeSyncModel& timenode)
{
  using namespace Scenario;
  // First we create a point for the timenode. The ingoing
  // intervals will end on this point.

  QString tn_name = name(timenode);
  ;

  if (timenode.active())
  {
     timenode.expression();
  }
  else
  {

  }


  auto prev_csts = previousIntervals(timenode, scenario);
  if (prev_csts.size() > 1)
  {
  }
  else if (prev_csts.size() == 1)
  {
  }
  else
  {
    SCORE_ABORT;
  }

}

void ISVisitor::visit(const Scenario::ScenarioInterface& scenario, const Scenario::EventModel& event)
{
  using namespace Scenario;
  const auto& timenode = parentTimeSync(event, scenario);
  QString tn_name = name(timenode);
  QString event_name = name(event);

}

void ISVisitor::visit(const Scenario::ScenarioInterface& scenario, const Scenario::StateModel& state) {}

void ISVisitor::visit(const Scenario::ScenarioInterface& s)
{

  /*
  using namespace Scenario;
  for (const auto& timenode : s.timeSyncs)
  {
    visit(s, timenode);
  }

  for (const auto& event : s.events)
  {
    visit(s, event);
  }

  for (const auto& state : s.states)
  {
    visit(s, state);
  }
  */

  for (const auto& interval : s.getIntervals())
  {
    visit(s, interval);
  }

}

void ISVisitor::visit(const Automation::ProcessModel& s)
{
  auto scenar = object("texture", name(s).toStdString());
  prop("name", "\"" + name(s).toStdString() + "\"");




}

void ISVisitor::visit(const Scenario::ScenarioInterface& scenario, const Scenario::IntervalModel& c)
{
  using namespace Scenario;
  auto& stn = startTimeSync(c, scenario);
  auto& sev = startEvent(c, scenario);
  QString parent_name = name(dynamic_cast<const QObject&>(scenario));
  QString start_event_name = name(startEvent(c, scenario));

  const TimeSyncModel& end_node = endTimeSync(c, scenario);
  QString end_node_name = name(end_node);

  QString cst_name = name(c);

  auto scenar = object("structure", name(c).toStdString());
  prop("name", "\"" + name(c).toStdString() + "\"");

  // Condition: all previous intervals finished
  if(auto* sc = dynamic_cast<const Scenario::ProcessModel*>(&scenario))
  {
    auto prev_itv = previousIntervals(stn, *sc);
    if(prev_itv.empty())
    {
      prop("start-when", "True");
    }
    else
    {
      std::string condition;
      for(const auto& id : prev_itv)
      {
        auto& itv = scenario.interval(id);
        condition += fmt::format("wait(start({}), {}, {}) /\\",
                                  name(itv).toStdString(),
                                 (int)itv.duration.minDuration().msec(),
                                 (int)itv.duration.maxDuration().msec()
                                 );
      }

      condition.resize(condition.size() - 3);
      prop("start-when", condition);
    }
    prop("stop-when", "True");
  }
  else
  {
    prop("start-when", "True");
    prop("stop-when", "True");
  }

  if(!c.processes.empty())
  {
    append("content: [");
    ++depth;

    if (c.duration.isRigid())
    {
      visitProcesses(c);
    }
    else
    {
      visitProcesses(c);
    }

    --depth;
    append("];");
  }
}


QString generateReactiveIS(const Scenario::ScenarioInterface& scenar, const Scenario::IntervalModel& root)
{
  ISVisitor v;
  v.visit(scenar, root);
  return QString::fromStdString(v.ss);
}

}
