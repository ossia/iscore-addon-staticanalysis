#include "ScenarioGenerator.hpp"
#include <Scenario/Commands/Scenario/Creations/CreateState.hpp>
#include <Scenario/Commands/Scenario/Creations/CreateConstraint_State_Event_TimeNode.hpp>
#include <Scenario/Commands/Scenario/Creations/CreateEvent_State.hpp>
#include <Scenario/Commands/Scenario/Creations/CreateConstraint.hpp>
#include <Scenario/Commands/TimeNode/AddTrigger.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Scenario/Document/TimeNode/TimeNodeModel.hpp>
#include <iscore/tools/ModelPathSerialization.hpp>
#include <Loop/LoopProcessModel.hpp>

#include <Scenario/Process/ScenarioProcessMetadata.hpp>
#include <Scenario/Commands/Constraint/AddProcessToConstraint.hpp>

#include <QFileDialog>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Scenario/Process/Algorithms/ContainersAccessors.hpp>
#include <Scenario/Commands/TimeNode/SetTrigger.hpp>
#include  <random>
#include  <iterator>
// http://stackoverflow.com/a/16421677/1495627
// https://gist.github.com/cbsmith/5538174
namespace stal
{

template <typename RandomGenerator = std::default_random_engine>
struct random_selector
{
        //On most platforms, you probably want to use std::random_device("/dev/urandom")()
        random_selector(RandomGenerator g = RandomGenerator(std::random_device()()))
            : gen(g) {}

        template <typename Iter>
        Iter select(Iter start, Iter end) {
            std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
            std::advance(start, dis(gen));
            return start;
        }

        //convenience function
        template <typename Iter>
        Iter operator()(Iter start, Iter end) {
            return select(start, end);
        }

        //convenience function that works on anything with a sensible begin() and end(), and returns with a ref to the value type
        template <typename Container> auto operator()(const Container& c) -> decltype(*begin(c))& {
            return *select(begin(c), end(c));
        }

    private:
        RandomGenerator gen;
};

class Transition{
  public:
    QString name;
};

auto createConstraint(
    CommandDispatcher<>& disp,
    const Scenario::ScenarioModel& scenario,
    const Scenario::StateModel& startState,
    double posY
    )
{

  using namespace Scenario;
  using namespace Scenario::Command;

  auto state_command = new CreateConstraint_State_Event_TimeNode(
              scenario,                   // scenario
              startState.id(),           // start state id
              Scenario::parentEvent(startState, scenario).date() + TimeValue::fromMsecs(2000), // duration
              posY);                       // y-pos in %
  disp.submitCommand(state_command);

  return state_command;
}

auto& createPlace(
    CommandDispatcher<>& disp,
    const Scenario::ScenarioModel& scenario,
    const Scenario::StateModel& startState,
    double posY
    )
{
  using namespace Scenario;
  using namespace Scenario::Command;

  // Create the synchronized event
  auto new_state_cmd = new CreateState(scenario, Scenario::parentEvent(startState, scenario).id(), posY);
  disp.submitCommand(new_state_cmd);
  auto& new_state = scenario.state(new_state_cmd->createdState());

  // Create constraint
  auto state_place_cmd = createConstraint(disp, scenario, new_state, posY);
  auto& state_place = scenario.state(state_place_cmd->createdState());

  // Create the loop
  using CreateProcess = AddProcessToConstraint<AddProcessDelegate<HasNoRacks>>;
  auto& new_constraint = scenario.constraint(state_place_cmd->createdConstraint());
  auto create_loop = new CreateProcess(
              new_constraint,
              Metadata<ConcreteFactoryKey_k, Loop::ProcessModel>::get());
  disp.submitCommand(create_loop);

  // Create loop constraint
  auto& loop = dynamic_cast<Loop::ProcessModel&>(new_constraint.processes.at(create_loop->processId()));
  auto& pattern = loop.constraint();

  auto create_scenario = new CreateProcess(
              pattern,
              Metadata<ConcreteFactoryKey_k, Scenario::ProcessModel>::get());
  disp.submitCommand(create_scenario);

  return state_place;
}

auto& createTransition(
    CommandDispatcher<>& disp,
    const Scenario::ScenarioModel& scenario,
    const Scenario::StateModel& startState,
    double posY
    )
{
  using namespace Scenario;
  using namespace Scenario::Command;

  // Create the constraint
  auto state_command = createConstraint(disp, scenario, startState, posY);
  auto& new_state = scenario.state(state_command->createdState());
  auto& new_timenode = parentTimeNode(new_state, scenario);

  // Create the trigger point
  auto trigger_command = new AddTrigger<Scenario::ScenarioModel>(new_timenode);
  disp.submitCommand(trigger_command);

  return new_state;
}


void generateScenarioFromPetriNet(
        const Scenario::ScenarioModel& scenario,
        CommandDispatcher<>& disp
        )
{
    using namespace Scenario;
    using namespace Scenario::Command;

    // search the file
    QString filename = QFileDialog::getOpenFileName();

    // load JSON file
    QFile jsonFile(filename);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)){
      qWarning("Couldn't open save file.");
      return;
    }
    QByteArray jsonData = jsonFile.readAll();
    jsonFile.close();
    QJsonDocument loadJson(QJsonDocument::fromJson(jsonData));
    QJsonObject json = loadJson.object();

    // Load Transitions
    QJsonArray transitionsArray = json["transitions"].toArray();
    QList<Transition> tList;
    for (int tIndex = 0; tIndex < transitionsArray.size(); ++tIndex) {
        QJsonObject tObject = transitionsArray[tIndex].toObject();
        qWarning() << tObject;
        Transition t;
        t.name = tObject["name"].toString();
        tList.append(t);
    }

    QString truc;
    auto elt_it = find_if(tList, [&] (auto elt) {
        return elt.name == truc;
    });
    if(elt_it != tList.end())
    {
        qDebug() << elt_it->name;
    }

    // initial state of the scenario
    auto& first_state = *states(scenario).begin();
    auto& state_initial_transition = createTransition(disp, scenario, first_state, 0.2);

    // create loops for each place
    // Load Places
    QJsonArray placesArray = json["places"].toArray();
    for (int pIndex = 0; pIndex < placesArray.size(); ++pIndex) {
        QJsonObject pObject = placesArray[pIndex].toObject();
        qWarning() << pObject;
        QVariantList pos_transitions = pObject["post"].toArray().toVariantList();
        QVariantList pre_transitions = pObject["pre"].toArray().toVariantList();

        if (pos_transitions.empty()){               // Final Place
          qWarning() << "It's a final place";
        } else if (pre_transitions.empty()){        // Initial Place
          qWarning() << "It's an initial place";
        } else {                                    // Segmentation Place
          double pos_y = pIndex * 0.1 + 0.1;
          auto& state_place = createPlace(disp, scenario, state_initial_transition, pos_y);
/*
          // Add post transitions of the place
          int indexT = tList.indexOf();
          if (indexT != -1){

          }
*/        }
    }

}

void generateScenario(
        const Scenario::ScenarioModel& scenar,
        int N,
        CommandDispatcher<>& disp)
{
    using namespace Scenario;
    disp.submitCommand(new Command::CreateState(scenar, scenar.startEvent().id(), 0.5));

    random_selector<> selector{};

    for(int i = 0; i < N; i++)
    {
        int randn = rand() % 2;
        double y = (rand() % 1000) / 1200.;
        switch(randn)
        {
            case 0:
            {
                // Get a random state to start from;
                StateModel& state = *selector(scenar.states.get());
                if(!state.nextConstraint())
                {
                    const TimeNodeModel& parentNode = parentTimeNode(state, scenar);
                    Id<StateModel> state_id = state.id();
                    TimeValue t = TimeValue::fromMsecs(rand() % 20000) + parentNode.date();
                    disp.submitCommand(new Command::CreateConstraint_State_Event_TimeNode(scenar, state_id, t, state.heightPercentage()));
                    break;
                }
            }
            case 1:
            {
                EventModel& event = *selector(scenar.events.get());
                if(&event != &scenar.endEvent())
                {
                    disp.submitCommand(new Command::CreateState(scenar, event.id(), y));
                }

                break;
            }
            default:
                break;
        }
    }
    for(int i = 0; i < N/3; i++)
    {
        StateModel& state1 = *selector(scenar.states.get());
        StateModel& state2 = *selector(scenar.states.get());
        auto& tn1 = Scenario::parentTimeNode(state1, scenar);
        auto t1 = tn1.date();
        auto& tn2 = Scenario::parentTimeNode(state2, scenar);
        auto t2 = tn2.date();
        if(t1 < t2)
        {
            if(!state2.previousConstraint() && !state1.nextConstraint())
                disp.submitCommand(new Command::CreateConstraint(scenar, state1.id(), state2.id()));
        }
        else if (t1 > t2)
        {
            if(!state1.previousConstraint() && !state2.nextConstraint())
                disp.submitCommand(new Command::CreateConstraint(scenar, state2.id(), state1.id()));
        }
    }
    for(auto i = 0; i < N/5; i++)
    {
        StateModel& state1 = *selector(scenar.states.get());
        auto& tn1 = Scenario::parentTimeNode(state1, scenar);

        if(tn1.hasTrigger())
            continue;
        disp.submitCommand(new Command::AddTrigger<ScenarioModel>(tn1));
    }
}
}
