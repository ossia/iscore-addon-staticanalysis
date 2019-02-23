#include "TAConversion.hpp"

#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Document/State/ItemModel/MessageItemModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Scenario/Process/ScenarioModel.hpp>

#include <ossia/detail/algorithms.hpp>
#include <ossia/network/value/value_conversion.hpp>

#include <QFile>
namespace stal
{
namespace TA
{
const int uppaal_division_factor
    = 100; // used because uppaal numbers don't go over 32768...
static int to_operator(ossia::expressions::comparator op)
{
  switch (op)
  {
    case ossia::expressions::comparator::EQUAL:
      return 1;
    case ossia::expressions::comparator::LOWER:
      return 2;
    case ossia::expressions::comparator::LOWER_EQUAL:
      return 3;
    case ossia::expressions::comparator::GREATER:
      return 4;
    case ossia::expressions::comparator::GREATER_EQUAL:
      return 5;
    default:
      return 0;
      // TODO !=
  }
}
static void set_point_condition(Point& point, const State::Expression& e)
{
  point.condition = 0;
  if (e.childCount() != 0)
  {
    auto& rel = e.childAt(0);
    if (rel.is<State::Relation>())
    {
      const State::Relation& r = rel.get<State::Relation>();
      auto val_ptr = r.rhs.target<ossia::value>();
      if (val_ptr)
      {
        point.condition = to_operator(r.op);

        auto val = ossia::convert<int>(*val_ptr);
        point.conditionValue = val;
      }
    }
  }
}

template <typename Container, typename Stream>
static void print(const Container& vec, Stream& s)
{
  for (const auto& elt : vec)
  {
    print(elt, s);
    s << "\n";
  }
  s << "\n";
}

template <typename Stream>
static void print(const Point& pt, Stream& stream)
{
  QString s = QString(
                  "%1 = Point(%2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, "
                  "%13);\n")
                  .arg(pt.name)
                  .arg(pt.condition)
                  .arg(pt.conditionValue / uppaal_division_factor)
                  .arg(pt.en)
                  .arg(pt.conditionMessage)
                  .arg(pt.event)
                  .arg(pt.urgent ? "true" : "false")
                  .arg(pt.event_s)
                  .arg(pt.skip_p)
                  .arg(pt.event_e)
                  .arg(pt.kill_p)
                  .arg(pt.skip)
                  .arg(pt.event_t);

  if (!pt.comment.isEmpty())
    stream << "// " << pt.comment.toLatin1().constData() << "\n";

  stream << s.toLatin1().constData();
}

template <typename Stream>
static void print(const Mix& pt, Stream& stream)
{
  QString s = QString("%1 = Mix(%2, %3, %4, %5);\n")
                  .arg(pt.name)
                  .arg(pt.event_in)
                  .arg(pt.event_out)
                  .arg(pt.skip_p)
                  .arg(pt.kill_p);

  stream << s.toLatin1().constData();
}

template <typename Stream>
static void print(const Control& pt, Stream& stream)
{
  QString s = QString("%1 = Control(%2, %3, %4, %5, %6, %7, %8);\n")
                  .arg(pt.name)
                  .arg(pt.num_prev_rels)
                  .arg(pt.event_s1)
                  .arg(pt.skip_p)
                  .arg(pt.skip)
                  .arg(pt.event_e)
                  .arg(pt.kill_p)
                  .arg(pt.event_s2);

  stream << s.toLatin1().constData();
}

template <typename Stream>
static void print(const Flexible& c, Stream& stream)
{
  QString s
      = QString(
            "%1 = Flexible(%2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12);\n")
            .arg(c.name)
            .arg((int)c.dmin.msec() / uppaal_division_factor)
            .arg(c.finite ? (int)c.dmax.msec() / uppaal_division_factor : 0)
            .arg(c.finite ? "true" : "false")
            .arg(c.event_s)
            .arg(c.event_min)
            .arg(c.event_i)
            .arg(c.event_max)
            .arg(c.skip_p)
            .arg(c.kill_p)
            .arg(c.skip)
            .arg(c.kill);

  if (!c.comment.isEmpty())
    stream << "// " << c.comment.toLatin1().constData() << "\n";

  stream << s.toLatin1().constData();
}

template <typename Stream>
static void print(const Rigid& c, Stream& stream)
{
  QString s = QString("%1 = Rigid(%2, %3, %4, %5, %6, %7, %8, %9);\n")
                  .arg(c.name)
                  .arg((int)c.dur.msec() / uppaal_division_factor)
                  .arg(c.event_s)
                  .arg(c.event_e1)
                  .arg(c.skip_p)
                  .arg(c.kill_p)
                  .arg(c.skip)
                  .arg(c.kill)
                  .arg(c.event_e2);

  if (!c.comment.isEmpty())
    stream << "// " << c.comment.toLatin1().constData() << "\n";
  stream << s.toLatin1().constData();
}

template <typename Stream>
static void print(const Event& c, Stream& stream)
{
  QString s = QString("%1 = Event(%2, %3, %4, %5);\n")
                  .arg(c.name)
                  .arg(c.message)
                  .arg(c.event)
                  .arg((int)c.date.msec() / uppaal_division_factor)
                  .arg(c.val);

  stream << s.toLatin1().constData();
}

template <typename Stream>
static void print(const Event_ND& c, Stream& stream)
{
  QString s = QString("%1 = Event_ND(%2, %3, %4, %5);\n")
                  .arg(c.name)
                  .arg(c.message)
                  .arg(c.event)
                  .arg((int)c.date.msec() / uppaal_division_factor)
                  .arg(c.val);

  stream << s.toLatin1().constData();
}

static QString print(const ScenarioContent& c)
{
  QFile f(":/model-uppaal.xml.in");
  SCORE_ASSERT(f.exists());
  f.open(QFile::ReadOnly);
  QString str = f.readAll();

  {
    std::stringstream output;

    output << "///// VARIABLES /////\n";
    for (const auto& elt : c.broadcasts)
      output << "broadcast chan " << qUtf8Printable(elt) << ";\n";
    for (const auto& elt : c.bools)
      output << "bool " << qUtf8Printable(elt) << ";\n";
    for (const auto& elt : c.ints)
      output << "int " << qUtf8Printable(elt) << ";\n";

    str.replace("$DECLARATIONS", QString::fromStdString(output.str()));
  }
  {
    std::stringstream output;
    output << "///// ELEMENTS /////\n";
    print(c.events, output);
    print(c.events_nd, output);
    print(c.rigids, output);
    print(c.flexibles, output);
    print(c.points, output);
    print(c.mixs, output);
    print(c.controls, output);

    output << "///// SYSTEM /////\n";
    output << "system\n";
    ossia::for_each_in_tuple(
        std::tie(
            c.events,
            c.events_nd,
            c.rigids,
            c.flexibles,
            c.points,
            c.mixs,
            c.controls),
        [&](const auto& vec) {
          for (const auto& elt : vec)
            output << qUtf8Printable(elt.name) << ",\n";
        });

    auto lastStr = QString::fromStdString(output.str());
    lastStr[lastStr.size() - 2] = ';';

    str.replace("$SYSTEM", lastStr);
  }

  return str;
}

static void insert(TA::ScenarioContent& source, TA::ScenarioContent& dest)
{
  dest.rigids.splice(dest.rigids.end(), source.rigids);
  dest.flexibles.splice(dest.flexibles.end(), source.flexibles);
  dest.points.splice(dest.points.end(), source.points);
  dest.events.splice(dest.events.end(), source.events);
  dest.events_nd.splice(dest.events_nd.end(), source.events_nd);
  dest.mixs.splice(dest.mixs.end(), source.mixs);
  dest.controls.splice(dest.controls.end(), source.controls);

  dest.bools.insert(source.bools.begin(), source.bools.end());
  dest.ints.insert(source.ints.begin(), source.ints.end());
  dest.broadcasts.insert(source.broadcasts.begin(), source.broadcasts.end());
}

template <typename T>
static void visitProcesses(
    const Scenario::IntervalModel& c,
    const T& ta_cst,
    TA::ScenarioContent& content)
{
  for (const auto& process : c.processes)
  {
    if (auto scenario = dynamic_cast<const Scenario::ProcessModel*>(&process))
    {
      TAVisitor v{*scenario, ta_cst};

      for (const TA::Point& point : v.scenario.points)
      {
        SCORE_ASSERT(!point.event_s.isEmpty());
        SCORE_ASSERT(!point.event_e.isEmpty());
        SCORE_ASSERT(!point.skip_p.isEmpty());
      }

      insert(v.scenario, content);
    }
  }
}

QString makeScenario(const Scenario::IntervalModel& c)
{
  using namespace Scenario;
  // Our register of elements
  ScenarioContent baseContent;

  // Global play
  TA::Event scenario_start_event{"MainStartEvent",
                                 IntVariable{"msg_start"},
                                 BroadcastVariable{"global_start"},
                                 TimeVal::zero(),
                                 1};
  TA::Event scenario_end_event{"MainEndEvent",
                               IntVariable{"msg_end"},
                               BroadcastVariable{"global_end"},
                               TimeVal::fromMsecs(18000),
                               1};

  QString cst_name = name(c);

  // Setup of the rigid
  TA::Flexible base{cst_name};
  base.dmin = TimeVal::zero();
  base.dmax = c.duration.maxDuration();
  base.finite = false;

  base.event_s = scenario_start_event.event;
  base.event_min = "event_e1" + cst_name;
  base.event_i = scenario_end_event.event;
  base.event_max = "event_e2" + cst_name;

  base.skip_p = "skip_p" + cst_name;
  base.kill_p = "kill_p" + cst_name;

  base.skip = "skip" + cst_name;
  base.kill = "kill" + cst_name;

  // Register all the new elements
  baseContent.flexibles.push_back(base);

  baseContent.broadcasts.insert(base.event_min);
  baseContent.broadcasts.insert(base.event_i);
  baseContent.broadcasts.insert(base.event_max);
  baseContent.broadcasts.insert(base.skip);
  baseContent.broadcasts.insert(base.kill);
  baseContent.broadcasts.insert(base.skip_p);
  baseContent.broadcasts.insert(base.kill_p);

  baseContent.events.push_back(scenario_start_event);
  baseContent.ints.insert(scenario_start_event.message);
  baseContent.events.push_back(scenario_end_event);
  baseContent.ints.insert(scenario_end_event.message);

  TA::Mix scenario_end_mix{
      "EndMix", base.event_max, base.kill, base.skip_p, base.kill_p};

  baseContent.mixs.push_back(scenario_end_mix);

  visitProcesses(c, base, baseContent);

  return print(baseContent);
}

const char* TAVisitor::space() const
{
  return qPrintable(QString(depth, '-'));
}

void TAVisitor::visit(const Scenario::TimeSyncModel& timenode)
{
  using namespace Scenario;
  // First we create a point for the timenode. The ingoing
  // intervals will end on this point.

  QString tn_name = name(timenode);
  // Create an interaction point.
  TA::Point tn_point{tn_name};

  tn_point.kill_p = scenario.kill;
  tn_point.en = "en_" + tn_name;
  tn_point.event = "event_" + tn_name;
  tn_point.skip = "skip_" + tn_name;
  tn_point.event_t = "ok_" + tn_name;

  if (timenode.active())
  {
    set_point_condition(tn_point, timenode.expression());
  }
  else
  {
    tn_point.condition = 0;
    tn_point.conditionValue = 0;
  }
  tn_point.conditionMessage = "msg" + tn_name;

  tn_point.urgent = true;

  scenario.bools.insert(tn_point.en);
  scenario.ints.insert(tn_point.conditionMessage);
  scenario.broadcasts.insert(tn_point.event);
  scenario.broadcasts.insert(tn_point.skip);
  scenario.broadcasts.insert(tn_point.event_t);

  // If there are multiple intervals ending on this timenode,
  // we put a Control inbetween.
  auto prev_csts = previousIntervals(timenode, scenario.score_scenario);
  if (prev_csts.size() > 1)
  {
    QString control_name = "Control_" + tn_name;
    TA::Control ctrl(
        control_name,
        prev_csts.size(),
        "event_s1_" + control_name,
        "skip_p_" + control_name,
        "skip_" + control_name,
        "event_e_" + control_name,
        scenario.kill,
        "event_s2_" + control_name);

    scenario.controls.push_back(ctrl);

    tn_point.skip_p = ctrl.skip;
    tn_point.event_e = ctrl.event_s2;
    tn_point.event_s = ctrl.event_e;

    scenario.broadcasts.insert(ctrl.event_s1);
    scenario.broadcasts.insert(ctrl.skip_p);
    scenario.broadcasts.insert(ctrl.skip);
    scenario.broadcasts.insert(ctrl.event_e);
    scenario.broadcasts.insert(ctrl.event_s2);
  }
  else if (prev_csts.size() == 1)
  {
    tn_point.skip_p = "skip_p_" + tn_name;
    tn_point.event_e = "event_e_" + tn_name;
    tn_point.event_s = "event_s_" + tn_name;

    scenario.broadcasts.insert(tn_point.skip_p);
    scenario.broadcasts.insert(tn_point.event_e);
    scenario.broadcasts.insert(tn_point.event_s);
  }
  else if (&timenode == &scenario.score_scenario.startTimeSync())
  {
    tn_point.skip_p = scenario.skip;
    tn_point.event_s = scenario.event_s;
    tn_point.event_e = scenario.event_s;
  }
  else
  {
    SCORE_ABORT;
  }

  // If there is a trigger we create a corresponding event.

  if (timenode.active())
  {
    TA::Event_ND node_event{"EventND_" + tn_name,
                            tn_point.conditionMessage,
                            tn_point.event,
                            timenode.date(), // TODO throw a rand
                            0};

    scenario.events_nd.push_back(node_event);
  }
  else
  {
    TA::Event node_event{"Event_" + tn_name,
                         tn_point.conditionMessage,
                         tn_point.event,
                         timenode.date(), // TODO throw a rand
                         0};

    scenario.events.push_back(node_event);
  }

  if (!timenode.active())
  {
    // TODO
    TA::Mix point_start_mix{"Mix_" + tn_name,
                            tn_point.event_s,
                            "mix_event_e" + tn_point.event_e,
                            tn_point.skip_p,
                            tn_point.kill_p};

    tn_point.event_e = point_start_mix.event_out;
    scenario.broadcasts.insert(point_start_mix.event_out);
    scenario.mixs.push_back(point_start_mix);
  }

  tn_point.comment = "TimeSync Name : " + timenode.metadata().getName()
                     + ". Label : " + timenode.metadata().getLabel();

  /*
  // We create a flexible that will go to each event of the timenode.
  QString flexible_name = "__after__" + tn_name;
  TA::Flexible flexible{flexible_name};

  flexible.dmin = TimeValue::zero();
  flexible.dmax = TimeValue::infinite();
  flexible.finite = false;

  flexible.event_s = tn_point.event_t;
  flexible.skip_p = scenario.skip;
  flexible.kill_p = scenario.kill;

  flexible.event_min = "emin_" + flexible_name;
  flexible.event_max = "emax_" + flexible_name;
  flexible.event_i = "event_" + flexible_name;

  flexible.skip = "skip_" + flexible_name;
  flexible.kill = "kill_" + flexible_name;

  scenario.flexibles.push_back(flexible);
  scenario.broadcasts.insert(flexible.event_min);
  scenario.broadcasts.insert(flexible.event_max);
  scenario.broadcasts.insert(flexible.skip);
  scenario.broadcasts.insert(flexible.kill);
*/
  scenario.points.push_back(tn_point);
}

void TAVisitor::visit(const Scenario::EventModel& event)
{
  using namespace Scenario;
  const auto& timenode = parentTimeSync(event, scenario.score_scenario);
  QString tn_name = name(timenode);
  auto it = ossia::find_if(scenario.points, [&](const auto& point) {
    return point.name == tn_name;
  });
  SCORE_ASSERT(it != scenario.points.end());

  const TA::Point& previous_timenode_point = *it;
  QString event_name = name(event);

  TA::Point point{event_name};

  // TODO condition
  // TODO states

  point.en = "en_" + event_name;
  point.skip = "skip_" + event_name;
  point.event = "event_" + event_name;
  point.event_t = "ok_" + event_name;
  point.event_e = "emax_" + event_name;

  set_point_condition(point, event.condition());
  point.conditionMessage = "msg" + event_name;

  point.event_s = previous_timenode_point.event_e;
  point.skip_p = previous_timenode_point.skip;

  point.kill_p = scenario.kill;

  point.urgent = true;

  point.comment = "EventNode Name : " + event.metadata().getName()
                  + ". Label : " + event.metadata().getLabel();
  if (!event.condition().hasChildren())
  {
    // No condition
    TA::Mix point_start_mix{"Mix_" + event_name,
                            previous_timenode_point.event_e,
                            point.event_e,
                            point.skip_p,
                            point.kill_p};

    scenario.mixs.push_back(point_start_mix);
  }

  TA::Event node_event{"Event_" + event_name,
                       point.conditionMessage,
                       point.event_e,
                       timenode.date(), // TODO throw a rand
                       0};

  scenario.events.push_back(node_event);

  scenario.points.push_back(point);

  scenario.bools.insert(point.en);
  scenario.ints.insert(point.conditionMessage);
  scenario.broadcasts.insert(point.skip);
  scenario.broadcasts.insert(point.event);
  scenario.broadcasts.insert(point.event_t);
  scenario.broadcasts.insert(point.event_e);

  // We already linked the start of this event, with
  // the end of the flexible created in the timenode pass
}

void TAVisitor::visit(const Scenario::StateModel& state) {}

void TAVisitor::visit(const Scenario::ProcessModel& s)
{
  using namespace Scenario;
  for (const TimeSyncModel& timenode : s.timeSyncs)
  {
    visit(timenode);
  }

  for (const EventModel& event : s.events)
  {
    visit(event);
  }

  for (const StateModel& state : s.states)
  {
    visit(state);
  }

  for (const IntervalModel& interval : s.intervals)
  {
    visit(interval);
  }
}

void TAVisitor::visit(const Scenario::IntervalModel& c)
{
  using namespace Scenario;
  QString start_event_name = name(startEvent(c, scenario.score_scenario));

  const TimeSyncModel& end_node = endTimeSync(c, scenario.score_scenario);
  QString end_node_name = name(end_node);

  auto prev_csts = previousIntervals(end_node, scenario.score_scenario);
  QString event_e1;
  QString event_e2;
  QString skip;
  if (prev_csts.size() > 1)
  {
    QString control_name = "Control_" + end_node_name;
    // We use the control.
    event_e1 = "event_s1_" + control_name;
    event_e2 = "event_s2_" + control_name;
    skip = "skip_p_" + control_name;
  }
  else
  {
    event_e1 = "event_s_" + end_node_name;
    event_e2 = "event_e_" + end_node_name;
    skip = "skip_p_" + end_node_name;
  }

  QString cst_name = name(c);

  if (c.duration.isRigid())
  {
    // Setup of the rigid
    TA::Rigid rigid{cst_name};
    rigid.dur = c.duration.defaultDuration();

    rigid.event_s = "ok_" + start_event_name;

    // skip_p : skip precedent
    // kill_p : kill parent
    rigid.skip_p
        = scenario.skip; // TODO this must be the skip of the start event.
    rigid.kill_p = scenario.kill;

    rigid.kill = "kill_" + cst_name;

    // Link with the end points
    rigid.event_e1 = event_e1;
    rigid.event_e2 = event_e2;
    rigid.skip = skip;

    // Register all the new elements
    rigid.comment = "Name : " + c.metadata().getName()
                    + ". Label : " + c.metadata().getLabel();
    scenario.rigids.push_back(rigid);
    scenario.broadcasts.insert(rigid.event_s);
    scenario.broadcasts.insert(rigid.skip);
    scenario.broadcasts.insert(rigid.kill);

    visitProcesses(c, rigid, scenario);
  }
  else
  {
    TA::Flexible flexible{cst_name};

    flexible.dmin = c.duration.minDuration();
    flexible.dmax = c.duration.maxDuration();
    flexible.finite = !c.duration.isMaxInfinite();

    flexible.event_s = "ok_" + start_event_name;
    flexible.skip_p
        = scenario.skip; // TODO this must be the skip of the start event.
    flexible.kill_p = scenario.kill;

    // From the point / control
    flexible.event_min = event_e1;
    flexible.event_max = event_e2;
    flexible.skip = skip;

    // TODO check this; this is weird because if we're in Control case there is
    // no event_e_Node anywhere."event_e_" + end_node_name;
    flexible.event_i = event_e2;

    QString cst_name = name(c);
    flexible.kill = "kill_" + cst_name;

    flexible.comment = "Name : " + c.metadata().getName()
                       + ". Label : " + c.metadata().getLabel();
    scenario.flexibles.push_back(flexible);
    scenario.broadcasts.insert(flexible.event_s);
    scenario.broadcasts.insert(flexible.skip);
    scenario.broadcasts.insert(flexible.kill);

    visitProcesses(c, flexible, scenario);
  }
}

}
}
