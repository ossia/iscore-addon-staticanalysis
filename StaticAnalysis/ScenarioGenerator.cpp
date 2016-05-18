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
#include <Scenario/Commands/Constraint/SetMaxDuration.hpp>
#include <Scenario/Commands/Constraint/SetMinDuration.hpp>

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

  friend bool operator==(const Transition& t, const QString& other)
  {
      return t.name == other;
  }

  friend bool operator!=(const Transition& t, const QString& other)
  {
      return t.name != other;
  }
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

  // Create the synchronized event
  auto new_state_cmd = new CreateState(scenario, Scenario::parentEvent(startState, scenario).id(), posY);
  disp.submitCommand(new_state_cmd);
  auto& new_state = scenario.state(new_state_cmd->createdState());

  auto state_command = new CreateConstraint_State_Event_TimeNode(
              scenario,                   // scenario
              new_state.id(),             // start state id
              Scenario::parentEvent(new_state, scenario).date() + TimeValue::fromMsecs(4000), // duration
              posY);                       // y-pos in %
  disp.submitCommand(state_command);

  return state_command;
}

template<typename Scenario_T>
void createTrigger(
        CommandDispatcher<>& disp,
        const Scenario::ScenarioModel& scenario,
        const Scenario::StateModel& state,
        const TimeValue& min_duration,
        const TimeValue& max_duration
        )
{
  using namespace Scenario;
  using namespace Scenario::Command;

  // Create the trigger point
  auto& timenode = parentTimeNode(state, scenario);
  auto trigger_command = new AddTrigger<Scenario_T>(timenode);
  disp.submitCommand(trigger_command);

  // Change Maximum Duration
  auto set_max_cmd = new SetMaxDuration(scenario.constraint(state.previousConstraint()), min_duration, true);
  disp.submitCommand(set_max_cmd);

   // Change Minimum Duration
   auto set_min_cmd = new SetMinDuration(scenario.constraint(state.previousConstraint()), max_duration, true);
   disp.submitCommand(set_min_cmd);
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

  // Create constraint
  auto state_place_cmd = createConstraint(disp, scenario, startState, posY);
  auto& loop_state = scenario.state(state_place_cmd->createdState());
  createTrigger(disp, scenario, loop_state, TimeValue::zero(), TimeValue::infinite());

  // Create the loop
  using CreateProcess = AddProcessToConstraint<AddProcessDelegate<HasNoRacks>>;
  auto& new_constraint = scenario.constraint(state_place_cmd->createdConstraint());
  auto create_loop_cmd = new CreateProcess(
              new_constraint,
              Metadata<ConcreteFactoryKey_k, Loop::ProcessModel>::get());
  disp.submitCommand(create_loop_cmd);

  // Create loop pattern
  auto& loop = dynamic_cast<Loop::ProcessModel&>(new_constraint.processes.at(create_loop_cmd->processId()));
  auto& pattern = loop.constraint();

  auto& pattern_state = loop.state(pattern.endState());
  createTrigger(disp, loop, pattern_state, TimeValue::zero(), TimeValue::infinite());

  auto create_scenario_cmd = new CreateProcess(
              pattern,
              Metadata<ConcreteFactoryKey_k, Scenario::ProcessModel>::get());
  disp.submitCommand(create_scenario_cmd);

  auto& scenario_pattern = static_cast<Scenario::ScenarioModel&>(pattern.processes.at(create_scenario_cmd->processId()));

  return scenario_pattern;
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

  // Create the trigger point
  createTrigger(disp, scenario, new_state, TimeValue::zero(), TimeValue::infinite());

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
        // qWarning() << tObject;
        Transition t;
        t.name = tObject["name"].toString();
        tList.append(t);
    }

    // initial state of the scenario
    auto& first_state = *states(scenario).begin();
    auto& state_initial_transition = createTransition(disp, scenario, first_state, 0.02);

    // create loops for each place
    // Load Places
    QJsonArray placesArray = json["places"].toArray();
    for (int pIndex = 0; pIndex < placesArray.size(); ++pIndex) {
        QJsonObject pObject = placesArray[pIndex].toObject();
        // qWarning() << pObject;
        QJsonArray pos_transitions = pObject["post"].toArray();
        QJsonArray pre_transitions = pObject["pre"].toArray();

        if (pos_transitions.empty()){               // Final Place
          qWarning() << "It's a final place";
        } else if (pre_transitions.empty()){        // Initial Place
          qWarning() << "It's an initial place";
        } else {                                    // Segmentation Place
          double pos_y = pIndex * 0.1 + 0.1;

          auto& scenario_place = createPlace(disp, scenario, state_initial_transition, pos_y);
          auto& start_state_id = *scenario_place.startEvent().states().begin();
          auto& place_start_state = scenario_place.states.at(start_state_id);;

          auto& state_transition = createTransition(disp, scenario_place, place_start_state, 0.4);

          // Add post transitions of the place
          for (int tIndex = 0; tIndex < pos_transitions.size(); ++tIndex) {
            QString tName = pos_transitions[tIndex].toString();
            auto elt_it = find(tList, tName);
            if (elt_it != tList.end()) {
                // Create the synchronized event
                double pos_t = tIndex * 0.4 + 0.8;

                createTransition(disp, scenario_place, state_transition, pos_t);

            }
          }
        }
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
