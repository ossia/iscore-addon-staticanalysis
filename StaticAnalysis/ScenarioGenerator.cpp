#include "ScenarioGenerator.hpp"
#include <Scenario/Commands/Scenario/Creations/CreateState.hpp>
#include <Scenario/Commands/Scenario/Creations/CreateInterval_State_Event_TimeSync.hpp>
#include <Scenario/Commands/Scenario/Creations/CreateEvent_State.hpp>
#include <Scenario/Commands/Scenario/Creations/CreateInterval.hpp>
#include <Scenario/Commands/TimeSync/AddTrigger.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <score/model/path/PathSerialization.hpp>
#include <Loop/LoopProcessModel.hpp>

#include <Scenario/Process/ScenarioProcessMetadata.hpp>
#include <Scenario/Commands/Interval/AddProcessToInterval.hpp>
#include <Scenario/Commands/Interval/SetMaxDuration.hpp>
#include <Scenario/Commands/Interval/SetMinDuration.hpp>

#include <QFileDialog>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Scenario/Process/Algorithms/ContainersAccessors.hpp>
#include <Scenario/Commands/TimeSync/SetTrigger.hpp>
#include <Explorer/Commands/Add/AddDevice.hpp>
#include <Explorer/Commands/Add/AddAddress.hpp>
#include <Protocols/OSC/OSCSpecificSettings.hpp>
#include <Protocols/OSC/OSCProtocolFactory.hpp>
#include <Protocols/OSC/OSCDevice.hpp>
#include <Device/Protocol/ProtocolList.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Scenario/Commands/State/AddMessagesToState.hpp>

#include <score/document/DocumentContext.hpp>
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
        template <typename Container> auto operator()(const Container& c) -> decltype(*begin(c).value().first)& {
            return *select(begin(c), end(c)).value().first;
        }

    private:
        RandomGenerator gen;
};

class Place{
  public:
    QString name;
    QList<QString> pre;
    QList<QString> pos;

  friend bool operator==(const Place& p, const QString& other)
  {
      return p.name == other;
  }

  friend bool operator!=(const Place& p, const QString& other)
  {
      return p.name != other;
  }

};

static auto& createTree(
        CommandDispatcher<>& disp,
        const score::DocumentContext& ctx)
{
    // Create a tree
    // Get necessary objects : OSC device factory, root node, etc.
    auto settings_factory = ctx.app.components
            .interfaces<Device::ProtocolFactoryList>()
            .get(Engine::Network::OSCProtocolFactory::static_concreteKey());

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
    theNode.clipMode = ossia::bounding_mode::FREE;
    theNode.ioType = ossia::access_mode::BI;
    theNode.value = value;

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
                state,
                { // A list
                    { // Of messages
                      State::AddressAccessor{ // The address
                        {device, {address}}
                      },
                      value // the value
                  }
                });

    disp.submitCommand(cmd);
}

static auto createInterval(
    CommandDispatcher<>& disp,
    const Scenario::ProcessModel& scenario,
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

  auto state_command = new CreateInterval_State_Event_TimeSync(
              scenario,                   // scenario
              new_state.id(),             // start state id
              Scenario::parentEvent(new_state, scenario).date() + TimeVal::fromMsecs(duration), // duration
              posY);                       // y-pos in %
  disp.submitCommand(state_command);

  return state_command;
}

template<typename Scenario_T>
void createTrigger(
        CommandDispatcher<>& disp,
        const Scenario_T& scenario,
        const Scenario::StateModel& state,
        const TimeVal& min_duration,
        const TimeVal& max_duration
        )
{
  using namespace Scenario;
  using namespace Scenario::Command;

  // Create the trigger point
  auto& timenode = parentTimeSync(state, scenario);
  auto trigger_command = new AddTrigger<Scenario_T>(timenode);
  disp.submitCommand(trigger_command);

   // Change Minimum Duration
   auto set_min_cmd = new SetMinDuration(scenario.interval(*state.previousInterval()), min_duration, min_duration.isZero());
   disp.submitCommand(set_min_cmd);

   // Change Maximum Duration
   auto set_max_cmd = new SetMaxDuration(scenario.interval(*state.previousInterval()), max_duration, max_duration.isInfinite());
   disp.submitCommand(set_max_cmd);
}

static auto createPlace(
    CommandDispatcher<>& disp,
    const Scenario::ProcessModel& scenario,
    const Scenario::StateModel& startState,
    double posY
    )
{
  using namespace Scenario;
  using namespace Scenario::Command;

  // Create interval
  auto state_place_cmd = createInterval(disp, scenario, startState, 12000, posY);
  auto& loop_state = scenario.state(state_place_cmd->createdState());
  createTrigger(disp, scenario, loop_state, TimeVal::zero(), TimeVal::infinite());

  // Create the loop
  auto& new_interval = scenario.interval(state_place_cmd->createdInterval());
  auto create_loop_cmd = new AddProcessToInterval(
              new_interval,
                           Metadata<ConcreteKey_k, Loop::ProcessModel>::get(), {});
  disp.submitCommand(create_loop_cmd);

  // Create loop pattern
  auto& loop = dynamic_cast<Loop::ProcessModel&>(new_interval.processes.at(create_loop_cmd->processId()));
  auto& pattern = loop.interval();

  auto& pattern_state = loop.state(pattern.endState());
  createTrigger(disp, loop, pattern_state, TimeVal::zero(), TimeVal::infinite());

  auto create_scenario_cmd = new AddProcessToInterval(
              pattern, Metadata<ConcreteKey_k, Scenario::ProcessModel>::get(), {});
  disp.submitCommand(create_scenario_cmd);

  auto& scenario_pattern = static_cast<Scenario::ProcessModel&>(pattern.processes.at(create_scenario_cmd->processId()));

  return std::tie(pattern_state, scenario_pattern, loop_state, loop);
}

template<typename Scenario_T>
void addConditionTrigger(
        CommandDispatcher<>& disp,
        const Scenario_T& scenario,
        const Scenario::StateModel& state,
        QList<QString> tList
        )
{
    using namespace Scenario;
    using namespace Scenario::Command;

    QList<QString> condition;
    Q_FOREACH (QString atom, tList){
        condition << "local:/" + atom + "==1";
    }
    QString expression = condition.join(" or ");

    // Try to parse an expression; if it is correctly parsed, add the time node
    auto maybe_parsed_expression = State::parseExpression(expression);
    if(maybe_parsed_expression)
    {
        // 4. Set the expression to the trigger
        auto& timenode = parentTimeSync(state, scenario);
        auto expr_command = new SetTrigger(timenode, *maybe_parsed_expression);
        disp.submitCommand(expr_command);
    }
}

static auto& createTransition(
        CommandDispatcher<>& disp,
        const Scenario::ProcessModel& scenario,
        const Scenario::StateModel& startState,
        const TimeVal& min_duration,
        const TimeVal& max_duration,
        double posY
    )
{
  using namespace Scenario;
  using namespace Scenario::Command;

  // Create the interval
  auto state_command = createInterval(disp, scenario, startState, 4000, posY);
  auto& new_state = scenario.state(state_command->createdState());

  // Create the trigger point
  createTrigger(disp, scenario, new_state, min_duration, max_duration);

  return new_state;
}


static QList<QString> JsonArrayToStringList(
        QJsonArray list)
{
    QList<QString> output;
    Q_FOREACH(QJsonValue s, list){
        output << s.toString();
    }
    return output;
}

static QJsonObject loadJsonFile(){
    // search the file
    QString filename = QFileDialog::getOpenFileName(NULL,
                                                    "Open Petri Net File",
                                                    QDir::currentPath(),
                                                    "JSON files (*.json)");

    // load JSON file
    QFile jsonFile(filename);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        qWarning("Couldn't open save file.");
        return QJsonObject();
    }

    QByteArray jsonData = jsonFile.readAll();
    jsonFile.close();

    QJsonDocument loadJson(QJsonDocument::fromJson(jsonData));
    return loadJson.object();
}

void generateScenarioFromPetriNet(
        const Scenario::ProcessModel& scenario,
        CommandDispatcher<>& disp
        )
{
    using namespace Scenario;
    using namespace Scenario::Command;

    // Load JSON file
    QJsonObject json = loadJsonFile();
    if (json.isEmpty()){ return; }

    // Create Device Tree
    auto& device_tree = createTree(disp, score::IDocument::documentContext(scenario));

    // Load Transitions
    QJsonArray transitionsArray = json["transitions"].toArray();
    for (int tIndex = 0; tIndex < transitionsArray.size(); ++tIndex) {
        QJsonObject tObject = transitionsArray[tIndex].toObject();
        createTreeNode(disp, device_tree, tObject["name"].toString(), false);  // adding transition variable to device tree
    }

    // default durations of transitions
    TimeVal t_min = TimeVal::fromMsecs(3000);
    TimeVal t_max = TimeVal::fromMsecs(5000);

    // initial state of the scenario
    auto& first_state = *states(scenario).begin();
    auto& state_initial_transition = createTransition(disp, scenario, first_state, t_min, t_max, 0.02);

    // Parsing Places from JSON file
    QJsonArray placesArray = json["places"].toArray();
    QList<Place> pList;
    QList<QString> finalTransitions, initialTransitions;
    for (int pIndex = 0; pIndex < placesArray.size(); ++pIndex) {
        QJsonObject pObject = placesArray[pIndex].toObject();
        Place p;
        p.name = pObject["name"].toString();
        p.pos = JsonArrayToStringList(pObject["post"].toArray());
        p.pre = JsonArrayToStringList(pObject["pre"].toArray());
        pList.append(p);

        if (p.pos.empty()){  // it's final place
            finalTransitions = p.pre;
        } else if (p.pre.empty()){  // it's initial place
            initialTransitions = p.pos;
        }
    }

    // create loops for each place
    double pos_y = 0.05;
    Q_FOREACH (Place p, pList) {
        if (!(p.pre.empty() || p.pos.empty())){

          auto place = createPlace(disp, scenario, state_initial_transition, pos_y);
          auto& scenario_place = std::get<1>(place);
          auto& pattern_state_place = std::get<0>(place);
          auto& loop_state = std::get<2>(place);
          auto& loop = std::get<3>(place);
          auto& start_state_id = *scenario_place.startEvent().states().begin();
          auto& place_start_state = scenario_place.states.at(start_state_id);

          auto& state_place = createTransition(disp, scenario_place, place_start_state,  TimeVal::zero(), TimeVal::infinite(), 0.4);

          // Add pre transitions of the place
          Q_FOREACH (QString t, p.pre){
              addMessageToState(disp, state_place, "local", t, false);
          }

          // Add post transitions of the place
          double pos_t = 0.8;
          Q_FOREACH (QString t, p.pos){
              auto& state_transition = createTransition(disp, scenario_place, state_place, t_min, t_max, pos_t);

              addMessageToState(disp, state_transition, "local", t, true);
              addMessageToState(disp, pattern_state_place, "local", t, false);

              pos_t += 0.4;
          }

          // Add stop condition of the place loop
          addConditionTrigger(disp, scenario, loop_state, finalTransitions);

          // Add stop condition of the loop pattern (post conditions)
          addConditionTrigger(disp, loop, pattern_state_place, p.pos);

          // Add conditon for start the loop (pre conditions)
          addConditionTrigger(disp, scenario_place, state_place, p.pre);

          // Increment pos Y
          pos_y += 0.1;
        }
    }
}

void generateScenario(
        const Scenario::ProcessModel& scenar,
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
                StateModel& state = selector(scenar.states.get());
                if(!state.nextInterval())
                {
                    const TimeSyncModel& parentNode = parentTimeSync(state, scenar);
                    Id<StateModel> state_id = state.id();
                    TimeVal t = TimeVal::fromMsecs(rand() % 20000) + parentNode.date();
                    disp.submitCommand(new Command::CreateInterval_State_Event_TimeSync(scenar, state_id, t, state.heightPercentage()));
                }
                break;
            }
            case 1:
            {
                EventModel& event = selector(scenar.events.get());
                disp.submitCommand(new Command::CreateState(scenar, event.id(), y));

                break;
            }
            default:
                break;
        }
    }
    for(int i = 0; i < N/3; i++)
    {
        StateModel& state1 = selector(scenar.states.get());
        StateModel& state2 = selector(scenar.states.get());
        auto& tn1 = Scenario::parentTimeSync(state1, scenar);
        auto t1 = tn1.date();
        auto& tn2 = Scenario::parentTimeSync(state2, scenar);
        auto t2 = tn2.date();
        if(t1 < t2)
        {
            if(!state2.previousInterval() && !state1.nextInterval())
                disp.submitCommand(new Command::CreateInterval(scenar, state1.id(), state2.id()));
        }
        else if (t1 > t2)
        {
            if(!state1.previousInterval() && !state2.nextInterval())
                disp.submitCommand(new Command::CreateInterval(scenar, state2.id(), state1.id()));
        }
    }
    for(auto i = 0; i < N/5; i++)
    {
        StateModel& state1 = selector(scenar.states.get());
        auto& tn1 = Scenario::parentTimeSync(state1, scenar);

        if(tn1.active())
            continue;
        disp.submitCommand(new Command::AddTrigger<ProcessModel>(tn1));
    }
}
}
