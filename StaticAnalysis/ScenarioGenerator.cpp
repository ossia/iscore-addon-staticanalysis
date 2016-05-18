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
#include <Explorer/Commands/Add/AddDevice.hpp>
#include <Explorer/Commands/Add/AddAddress.hpp>
#include <OSSIA/Protocols/OSC/OSCSpecificSettings.hpp>
#include <OSSIA/Protocols/OSC/OSCProtocolFactory.hpp>
#include <OSSIA/Protocols/OSC/OSCDevice.hpp>
#include <Device/Protocol/ProtocolList.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Scenario/Commands/State/AddMessagesToState.hpp>

#include <iscore/document/DocumentContext.hpp>
#include <tuple>
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

auto& createTree(
        CommandDispatcher<>& disp,
        const iscore::DocumentContext& ctx)
{
    // Create a tree
    // Get necessary objects : OSC device factory, root node, etc.
    auto settings_factory = ctx.app.components
            .factory<Device::DynamicProtocolList>()
            .get(Ossia::OSCProtocolFactory::static_concreteFactoryKey());

    auto& tree = ctx.plugin<Explorer::DeviceDocumentPlugin>();

    // First create a device
    auto settings = settings_factory->defaultSettings();
    settings.name = "local";
    auto create_dev_cmd = new Explorer::Command::AddDevice(tree, settings);
    disp.submitCommand(create_dev_cmd);

    return tree;
}

template<typename Val>
auto createTreeNode(
        CommandDispatcher<>& disp,
        Explorer::DeviceDocumentPlugin& tree,
        QString name,
        Val&& value
        )
{
    // Find the created device
    auto& root = tree.rootNode();
    auto& created_device_root = Device::getNodeFromAddress(root, State::Address{"local", {}});

    // Create some node
    Device::AddressSettings theNode;
    theNode.name = name;
    theNode.clipMode = Device::ClipMode::Free;
    theNode.ioType = Device::IOType::InOut;
    theNode.value = State::Value::fromValue(value);

    auto create_addr_cmd = new Explorer::Command::AddAddress(
                tree,
                created_device_root,
                InsertMode::AsChild,
                theNode);

    disp.submitCommand(create_addr_cmd);
}

template<typename Val>
auto addMessageToState(
        CommandDispatcher<>& disp,
        Scenario::StateModel& state,
        QString device,
        QString address,
        Val&& value
        )
{
    auto cmd = new Scenario::Command::AddMessagesToState(
                state.messages(),
                { // A list
                    { // Of messages
                      { // The address
                          device, {address}
                      },
                      State::Value::fromValue(value) // the value
                  }
                });

    disp.submitCommand(cmd);
}

auto createConstraint(
    CommandDispatcher<>& disp,
    const Scenario::ScenarioModel& scenario,
    const Scenario::StateModel& startState,
    const int duration,
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
              Scenario::parentEvent(new_state, scenario).date() + TimeValue::fromMsecs(duration), // duration
              posY);                       // y-pos in %
  disp.submitCommand(state_command);

  return state_command;
}

template<typename Scenario_T>
void createTrigger(
        CommandDispatcher<>& disp,
        const Scenario_T& scenario,
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

   // Change Minimum Duration
   auto set_min_cmd = new SetMinDuration(scenario.constraint(state.previousConstraint()), min_duration, min_duration.isZero());
   disp.submitCommand(set_min_cmd);

   // Change Maximum Duration
   auto set_max_cmd = new SetMaxDuration(scenario.constraint(state.previousConstraint()), max_duration, max_duration.isInfinite());
   disp.submitCommand(set_max_cmd);
}

auto createPlace(
    CommandDispatcher<>& disp,
    const Scenario::ScenarioModel& scenario,
    const Scenario::StateModel& startState,
    double posY
    )
{
  using namespace Scenario;
  using namespace Scenario::Command;

  // Create constraint
  auto state_place_cmd = createConstraint(disp, scenario, startState, 12000, posY);
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

  return std::tie(pattern_state, scenario_pattern);
}

auto& createTransition(
        CommandDispatcher<>& disp,
        const Scenario::ScenarioModel& scenario,
        const Scenario::StateModel& startState,
        const TimeValue& min_duration,
        const TimeValue& max_duration,
        double posY
    )
{
  using namespace Scenario;
  using namespace Scenario::Command;

  // Create the constraint
  auto state_command = createConstraint(disp, scenario, startState, 4000, posY);
  auto& new_state = scenario.state(state_command->createdState());

  // Create the trigger point
  createTrigger(disp, scenario, new_state, min_duration, max_duration);

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
    QString filename = QFileDialog::getOpenFileName(NULL, "Open Petri Net File", QDir::currentPath(), "JSON files (*.json)");

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

    // Create Device Tree
    auto& device_tree = createTree(disp, iscore::IDocument::documentContext(scenario));

    // Load Transitions
    QJsonArray transitionsArray = json["transitions"].toArray();
//    QList<Transition> tList;
    for (int tIndex = 0; tIndex < transitionsArray.size(); ++tIndex) {
        QJsonObject tObject = transitionsArray[tIndex].toObject();
//        Transition t;
//        t = tObject["name"].toString();
        createTreeNode(disp, device_tree, tObject["name"].toString(), false);  // adding transition variable to device tree
//        tList.append(t);
    }

    // default durations of transitions
    TimeValue t_min = TimeValue::fromMsecs(3000);
    TimeValue t_max = TimeValue::fromMsecs(5000);


    // initial state of the scenario
    auto& first_state = *states(scenario).begin();
    auto& state_initial_transition = createTransition(disp, scenario, first_state, t_min, t_max, 0.02);

    // create loops for each place
    // Load Places
    QJsonArray placesArray = json["places"].toArray();
    double pos_y = 0.05;
    for (int pIndex = 0; pIndex < placesArray.size(); ++pIndex) {
        QJsonObject pObject = placesArray[pIndex].toObject();
        // qWarning() << pObject;
        QJsonArray pos_transitions = pObject["post"].toArray();
        QJsonArray pre_transitions = pObject["pre"].toArray();

        if (!(pos_transitions.empty() || pre_transitions.empty())){

          auto place = createPlace(disp, scenario, state_initial_transition, pos_y);
          auto& scenario_place = std::get<1>(place);
          auto& pattern_state_place = std::get<0>(place);
          auto& start_state_id = *scenario_place.startEvent().states().begin();
          auto& place_start_state = scenario_place.states.at(start_state_id);

          auto& state_place = createTransition(disp, scenario_place, place_start_state,  TimeValue::zero(), TimeValue::infinite(), 0.4);

          // Add pre transitions of the place
          for (int tIndex = 0; tIndex < pre_transitions.size(); ++tIndex) {
              QString tName = pre_transitions[tIndex].toString();
              addMessageToState(disp, state_place, "local", tName, false);
          }

          // Add post transitions of the place
          for (int tIndex = 0; tIndex < pos_transitions.size(); ++tIndex) {
            QString tName = pos_transitions[tIndex].toString();
//            auto elt_it = find(tList, tName);
//            if (elt_it != tList.end()) {}

            // Create the synchronized event
            double pos_t = tIndex * 0.4 + 0.8;

            auto& state_transition = createTransition(disp, scenario_place, state_place, t_min, t_max, pos_t);

            addMessageToState(disp, state_transition, "local", tName, true);
            addMessageToState(disp, pattern_state_place, "local", tName, false);

          }

          // Increment pos Y
          pos_y += 0.1;
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
