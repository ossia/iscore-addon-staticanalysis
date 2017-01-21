#pragma once
#include <QString>
#include <sstream>
#include <Process/TimeValue.hpp>
#include <iscore/model/path/Path.hpp>
#include <set>
#include <eggs/variant.hpp>

#include <Scenario/Document/TimeNode/TimeNodeModel.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/State/StateModel.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Process/ScenarioProcessMetadata.hpp>
namespace Scenario
{
class TimeNodeModel;
class EventModel;
class ConstraintModel;
class StateModel;
class ProcessModel;
}
namespace stal
{
namespace TA
{
struct TAScenario;

template<typename Object>
QString name(const Object& obj)
{
    ObjectPath path = Path<Object>{obj}.unsafePath();
    path.vec().erase(path.vec().begin(), path.vec().begin() + 3);
    std::reverse(path.vec().begin(), path.vec().end());
    for(ObjectIdentifier& elt : path.vec())
    {
        if(elt.objectName() == Metadata<ObjectKey_k, Scenario::ProcessModel>::get())
            elt = ObjectIdentifier{"S", elt.id()};
        else if(elt.objectName() == Metadata<ObjectKey_k, Scenario::EventModel>::get())
            elt = ObjectIdentifier{"E", elt.id()};
        else if(elt.objectName().contains("Constraint"))
            elt = ObjectIdentifier{"C", elt.id()};
        else if(elt.objectName() == Metadata<ObjectKey_k, Scenario::TimeNodeModel>::get())
            elt = ObjectIdentifier{"T", elt.id()};
    }

    return path.toString().replace('/', "_").replace('.', "").prepend('_');
}

using BroadcastVariable = QString;
using BoolVariable = QString;
using IntVariable = QString;
struct Event
{
    Event(QString n, IntVariable a, BroadcastVariable b, TimeVal c, int d):
        name{n},
        message{a},
        event{b},
        date{c},
        val{d}
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
    Event_ND(QString n, IntVariable a, BroadcastVariable b, TimeVal c, int d):
        name{n},
        message{a},
        event{b},
        date{c},
        val{d}
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
            BroadcastVariable d):
            name{name},
            event_in{a},
            event_out{b},
            skip_p{c},
            kill_p{d}
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
    Control(QString name,
            int num,
            BroadcastVariable a,
            BroadcastVariable b,
            BroadcastVariable c,
            BroadcastVariable d,
            BroadcastVariable e,
            BroadcastVariable f
            ):
        name{name},
        num_prev_rels{num},
        event_s1{a},
        skip_p{b},
        skip{c},
        event_e{d},
        kill_p{e},
        event_s2{f}
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
    Point(const QString& name):
        name{name}
    {

    }

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
    Flexible(const QString& name):
        name{name}
    {

    }

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
    Rigid(const QString& name):
        name{name}
    {

    }

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

using Constraint = eggs::variant<Rigid, Flexible>;

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
    template<typename T>
    TAScenario(const Scenario::ProcessModel& s, const T& constraint):
        iscore_scenario{s},
        self{constraint},
        event_s{constraint.event_s},
        skip{constraint.skip},
        kill{constraint.kill}
    {
        broadcasts.insert(event_s);
        broadcasts.insert(skip);
        broadcasts.insert(kill);
    }

    const Scenario::ProcessModel& iscore_scenario;

    TA::Constraint self; // The scenario is considered similar to a constraint.

    const TA::BroadcastVariable event_s;// = "skip_S" + name(iscore_scenario);
    const TA::BroadcastVariable skip;// = "skip_S" + name(iscore_scenario);
    const TA::BroadcastVariable kill;// = "kill_S" + name(iscore_scenario);
};

struct TAVisitor
{
    TA::TAScenario scenario;
    template<typename T>
    TAVisitor(const Scenario::ProcessModel& s, const T& constraint):
        scenario{s, constraint}
    {
        visit(s);
    }

private:
    int depth = 0;
    const char* space() const;

    void visit(
            const Scenario::TimeNodeModel& timenode);
    void visit(
            const Scenario::EventModel& event);
    void visit(
            const Scenario::ConstraintModel& c);
    void visit(
            const Scenario::StateModel& state);
    void visit(
            const Scenario::ProcessModel& s);
};

QString makeScenario(const Scenario::ConstraintModel& s);
}
}
