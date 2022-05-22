#pragma once
#include <Process/TimeValue.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/State/StateModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Process/ScenarioProcessMetadata.hpp>

#include <score/model/path/Path.hpp>

#include <QString>

#include <ossia/detail/variant.hpp>

#include <set>
#include <sstream>
namespace Scenario
{
class TimeSyncModel;
class EventModel;
class IntervalModel;
class StateModel;
class ProcessModel;
}
namespace stal
{
namespace TA
{
struct TAScenario;


inline QString name(const QObject& obj)
{
  ObjectPath path = score::IDocument::unsafe_path(obj);

  path.vec().erase(path.vec().begin(), path.vec().begin() + 2);
  std::reverse(path.vec().begin(), path.vec().end());
  for (ObjectIdentifier& elt : path.vec())
  {
    if (elt.objectName()
        == Metadata<ObjectKey_k, Scenario::ProcessModel>::get())
      elt = ObjectIdentifier{"S", elt.id()};
    else if (
        elt.objectName() == Metadata<ObjectKey_k, Scenario::EventModel>::get())
      elt = ObjectIdentifier{"E", elt.id()};
    else if (elt.objectName().contains("Interval"))
      elt = ObjectIdentifier{"C", elt.id()};
    else if (
        elt.objectName()
        == Metadata<ObjectKey_k, Scenario::TimeSyncModel>::get())
      elt = ObjectIdentifier{"T", elt.id()};
  }

  auto str = path.toString().replace('/', "_").replace('.', "");
  str.resize(str.size() - 1);
  return str;
}

using BroadcastVariable = QString;
using BoolVariable = QString;
using IntVariable = QString;
struct Event
{
  Event(QString n, IntVariable a, BroadcastVariable b, TimeVal c, int d)
      : name{n}, message{a}, event{b}, date{c}, val{d}
  {
  }

  QString name;
  IntVariable message{};
  BroadcastVariable event{};
  TimeVal date{};
  int val{};
};

struct Event_ND
{
  Event_ND(QString n, IntVariable a, BroadcastVariable b, TimeVal c, int d)
      : name{n}, message{a}, event{b}, date{c}, val{d}
  {
  }

  QString name;
  IntVariable message{};
  BroadcastVariable event{};
  TimeVal date{};
  int val{};
};

struct Mix
{
  Mix(QString name,
      BroadcastVariable a,
      BroadcastVariable b,
      BroadcastVariable c,
      BroadcastVariable d)
      : name{name}, event_in{a}, event_out{b}, skip_p{c}, kill_p{d}
  {
  }

  QString name;
  BroadcastVariable event_in;
  BroadcastVariable event_out;
  BroadcastVariable skip_p;
  BroadcastVariable kill_p;
};

struct Control
{
  Control(
      QString name,
      int num,
      BroadcastVariable a,
      BroadcastVariable b,
      BroadcastVariable c,
      BroadcastVariable d,
      BroadcastVariable e,
      BroadcastVariable f)
      : name{name}
      , num_prev_rels{num}
      , event_s1{a}
      , skip_p{b}
      , skip{c}
      , event_e{d}
      , kill_p{e}
      , event_s2{f}
  {
  }

  QString name;
  int num_prev_rels = 0;
  BroadcastVariable event_s1;
  BroadcastVariable skip_p;
  BroadcastVariable skip;
  BroadcastVariable event_e;
  BroadcastVariable kill_p;
  BroadcastVariable event_s2;
};

struct Point
{
  Point(const QString& name) : name{name} {}

  QString name;
  int condition{};
  int conditionValue{};
  BoolVariable en; // Enabled
  IntVariable conditionMessage;

  BroadcastVariable event;

  bool urgent = true;

  BroadcastVariable event_s;
  BroadcastVariable skip_p;
  BroadcastVariable event_e;
  BroadcastVariable kill_p;

  BroadcastVariable skip;
  BroadcastVariable event_t;

  QString comment;
};

struct Flexible
{
  Flexible(const QString& name) : name{name} {}

  QString name;

  TimeVal dmin;
  TimeVal dmax;
  bool finite = true;

  BroadcastVariable event_s;
  BroadcastVariable event_min;
  BroadcastVariable event_i;
  BroadcastVariable event_max;

  BroadcastVariable skip_p;
  BroadcastVariable kill_p;
  BroadcastVariable skip;
  BroadcastVariable kill;

  QString comment;
};

struct Rigid
{
  Rigid(const QString& name) : name{name} {}

  QString name;

  TimeVal dur;

  BroadcastVariable event_s;
  BroadcastVariable event_e1;

  BroadcastVariable skip_p;
  BroadcastVariable kill_p;
  BroadcastVariable skip;
  BroadcastVariable kill;

  BroadcastVariable event_e2;

  QString comment;
};

using Interval = ossia::variant<Rigid, Flexible>;

struct ScenarioContent
{
  std::set<TA::BroadcastVariable> broadcasts;
  std::set<TA::IntVariable> ints;
  std::set<TA::BoolVariable> bools;

  std::list<TA::Rigid> rigids;
  std::list<TA::Flexible> flexibles;
  std::list<TA::Point> points;
  std::list<TA::Event> events;
  std::list<TA::Event_ND> events_nd;
  std::list<TA::Mix> mixs;
  std::list<TA::Control> controls;
};

struct TAScenario : public ScenarioContent
{
  template <typename T>
  TAScenario(const Scenario::ProcessModel& s, const T& interval)
      : score_scenario{s}
      , self{interval}
      , event_s{interval.event_s}
      , skip{interval.skip}
      , kill{interval.kill}
  {
    broadcasts.insert(event_s);
    broadcasts.insert(skip);
    broadcasts.insert(kill);
  }

  const Scenario::ProcessModel& score_scenario;

  TA::Interval self; // The scenario is considered similar to a interval.

  const TA::BroadcastVariable event_s; // = "skip_S" + name(score_scenario);
  const TA::BroadcastVariable skip;    // = "skip_S" + name(score_scenario);
  const TA::BroadcastVariable kill;    // = "kill_S" + name(score_scenario);
};

struct TAVisitor
{
  TA::TAScenario scenario;
  template <typename T>
  TAVisitor(const Scenario::ProcessModel& s, const T& interval)
      : scenario{s, interval}
  {
    visit(s);
  }

private:
  int depth = 0;
  const char* space() const;

  void visit(const Scenario::TimeSyncModel& timenode);
  void visit(const Scenario::EventModel& event);
  void visit(const Scenario::IntervalModel& c);
  void visit(const Scenario::StateModel& state);
  void visit(const Scenario::ProcessModel& s);
};

QString makeScenario(const Scenario::IntervalModel& s);
}
}
