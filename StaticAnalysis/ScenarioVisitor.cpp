#include <Process/Process.hpp>
#include <Process/State/MessageNode.hpp>
#include <Scenario/Application/Menus/TextDialog.hpp>
#include <Scenario/Document/BaseScenario/BaseScenario.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/Interval/IntervalDurations.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Document/State/ItemModel/MessageItemModel.hpp>
#include <Scenario/Document/State/StateModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Scenario/Process/ScenarioModel.hpp>
#include <State/Message.hpp>
#include <State/Value.hpp>

#include <score/actions/ActionManager.hpp>
#include <score/actions/Menu.hpp>
#include <score/actions/MenuManager.hpp>
#include <score/document/DocumentInterface.hpp>
#include <score/model/Identifier.hpp>
#include <score/plugins/application/GUIApplicationPlugin.hpp>

#include <core/document/Document.hpp>

#include <ossia/detail/algorithms.hpp>

#include <QAction>
#include <QApplication>
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QJsonDocument>
#include <QSaveFile>
#include <QString>

#include <StaticAnalysis/CppGenerator.hpp>
#include <StaticAnalysis/ScenarioGenerator.hpp>
#include <StaticAnalysis/ScenarioMetrics.hpp>
#include <StaticAnalysis/ScenarioVisitor.hpp>
#include <StaticAnalysis/Statistics.hpp>
#include <StaticAnalysis/TAConversion.hpp>
#include <StaticAnalysis/TIKZConversion.hpp>

#include <sstream>

stal::ApplicationPlugin::ApplicationPlugin(
    const score::GUIApplicationContext& app)
    : score::GUIApplicationPlugin{app}
{
  m_himito = new QAction{tr("Generate scenario from Petri Net"), nullptr};
  connect(m_himito, &QAction::triggered, [&]() {
    auto doc = currentDocument();
    if (!doc)
      return;
    Scenario::ScenarioDocumentModel& base
        = score::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);

    const auto& baseInterval = base.baseScenario().interval();
    if (baseInterval.processes.size() == 0)
      return;

    auto firstScenario = dynamic_cast<Scenario::ProcessModel*>(
        &*baseInterval.processes.begin());
    if (!firstScenario)
      return;

    CommandDispatcher<> disp(doc->context().commandStack);

    stal::generateScenarioFromPetriNet(*firstScenario, disp);
  });
  m_generate = new QAction{tr("Generate random score"), nullptr};
  connect(m_generate, &QAction::triggered, [&]() {
    auto doc = currentDocument();
    if (!doc)
      return;
    Scenario::ScenarioDocumentModel& base
        = score::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);

    const auto& baseInterval = base.baseScenario().interval();
    if (baseInterval.processes.size() == 0)
      return;

    auto firstScenario = dynamic_cast<Scenario::ProcessModel*>(
        &*baseInterval.processes.begin());
    if (!firstScenario)
      return;

    CommandDispatcher<> disp(doc->context().commandStack);
    /*
    for(int set = 0; set < 100; set++)
    {
        for(int n = 5; n < 80; n++)
        {
            while(doc->commandStack().canUndo())
                doc->commandStack().undo();

            stal::generateScenario(*firstScenario, n, disp);

            QString baseName = "data/" + QString::number(set) + "/" +
    QString::number(n);
            {
                QFile savefile(baseName + ".scorejson");
                savefile.open(QFile::WriteOnly);
                QJsonDocument jdoc(doc->saveAsJson());

                savefile.write(jdoc.toJson());
                savefile.close();
            }

            {
                QString text =
    TA::makeScenario(base.baseScenario().interval()); QFile f(baseName +
    ".xml"); f.open(QFile::WriteOnly); f.write(text.toUtf8()); f.close();
            }
        }
    }
        */
    stal::generateScenario(*firstScenario, 300, disp);
  });
  m_convert = new QAction{tr("Convert to Temporal Automatas"), nullptr};
  connect(m_convert, &QAction::triggered, [&]() {
    auto doc = currentDocument();
    if (!doc)
      return;
    Scenario::ScenarioDocumentModel& base
        = score::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);

    QString text = TA::makeScenario(base.baseScenario().interval());
    Scenario::TextDialog dial(text, qApp->activeWindow());
    dial.exec();

    QFile f("model-output.xml");
    f.open(QFile::WriteOnly);
    f.write(text.toUtf8());
    f.close();
  });

  m_metrics = new QAction{tr("Scenario metrics"), nullptr};
  connect(m_metrics, &QAction::triggered, [&]() {
    auto doc = currentDocument();
    if (!doc)
      return;

    Scenario::ScenarioDocumentModel& base
        = score::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);
    auto& baseScenario = static_cast<Scenario::ProcessModel&>(
        *base.baseScenario().interval().processes.begin());

    using namespace stal::Metrics;
    // Language
    QString str = toScenarioLanguage(baseScenario);

    // Halstead
    {
      auto factors = Halstead::ComputeFactors(baseScenario);
      str += "Difficulty = " + QString::number(Halstead::Difficulty(factors))
             + "\n";
      str += "Volume = " + QString::number(Halstead::Volume(factors)) + "\n";
      str += "Effort = " + QString::number(Halstead::Effort(factors)) + "\n";
      str += "TimeRequired = "
             + QString::number(Halstead::TimeRequired(factors)) + "\n";
      str += "Bugs2 = " + QString::number(Halstead::Bugs2(factors)) + "\n";
    }
    // Cyclomatic
    {
      auto factors = Cyclomatic::ComputeFactors(baseScenario);
      str += "Cyclomatic1 = "
             + QString::number(Cyclomatic::Complexity(factors));
    }
    // Display
    Scenario::TextDialog dial(str, qApp->activeWindow());
    dial.exec();
  });

  m_TIKZexport = new QAction{tr("Export in TIKZ"), nullptr};
  connect(m_TIKZexport, &QAction::triggered, [&]() {
    auto doc = currentDocument();
    if (!doc)
      return;
    Scenario::ScenarioDocumentModel& base
        = score::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);
    auto& baseScenario = static_cast<Scenario::ProcessModel&>(
        *base.baseScenario().interval().processes.begin());

    QFileDialog d{qApp->activeWindow(), tr("Save Document As")};
    d.setNameFilter(("tex files (*.tex)"));
    d.setConfirmOverwrite(true);
    d.setFileMode(QFileDialog::AnyFile);
    d.setAcceptMode(QFileDialog::AcceptSave);

    if (d.exec())
    {
      auto files = d.selectedFiles();
      QString savename = files.first();
      QString name = savename;
      name.remove(0, savename.lastIndexOf("/") + 1);
      name.remove(".tex");
      if (!savename.isEmpty())
      {
        if (!savename.contains(".tex"))
          savename += ".tex";

        QSaveFile f{savename};
        f.open(QIODevice::WriteOnly);

        QString tex = makeTIKZ2(name, baseScenario);
        f.write(tex.toUtf8());
        f.commit();
      }
    }
  });

  m_statistics = new QAction{tr("Statistics"), nullptr};
  connect(
      m_statistics, &QAction::triggered, [&]() {
        auto doc = currentDocument();
        Scenario::ScenarioDocumentModel& base
            = score::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);
        ExplorerStatistics e{
            doc->context().plugin<Explorer::DeviceDocumentPlugin>()};
        GlobalStatistics g{base.baseInterval()};

        QString str;
        str += "Devices\n=======\n\n";
        str += "Device Nodes NonLeaf MaxDepth MaxCld AvgCld AvgNCld\n";
        for (const DeviceStatistics& dev : e.devices)
        {

          str += dev.name + " ";
          str += QString::number(dev.nodes) + " ";
          str += QString::number(dev.non_leaf_nodes) + " ";
          str += QString::number(dev.max_depth) + " ";
          str += QString::number(dev.max_child_count) + " ";
          str += QString::number(dev.avg_child_count) + " ";
          str += QString::number(dev.avg_non_leaf_child_count) + " ";
          str += "\n";
        }

        str += "\n\n";
        str += "Device Empty Int Impulse Float Bool Vec2F Vec3F Vec4F Tuple String Char\n";
        for (const DeviceStatistics& dev : e.devices)
        {
          str += dev.name + " ";
          str += QString::number(dev.containers) + " ";
          str += QString::number(dev.int_addr) + " ";
          str += QString::number(dev.impulse_addr) + " ";
          str += QString::number(dev.float_addr) + " ";
          str += QString::number(dev.bool_addr) + " ";
          str += QString::number(dev.vec2f_addr) + " ";
          str += QString::number(dev.vec3f_addr) + " ";
          str += QString::number(dev.vec4f_addr) + " ";
          str += QString::number(dev.tuple_addr) + " ";
          str += QString::number(dev.string_addr) + " ";
          str += QString::number(dev.char_addr) + " ";
          str += "\n";
          /*
                  str += "Get      :\t" + QString::number(dev.num_get) + "\n";
                  str += "Set      :\t" + QString::number(dev.num_set) + "\n";
                  str += "Bi       :\t" + QString::number(dev.num_bi) + "\n";
                  str += "\n";

                  str += "AMetadata:\t" + QString::number(dev.avg_ext_metadata)
             + "\n";
                  */
        }

        str += "\n\n";
        str += "Score\n=======\n\n";
        str += "Intervals EmptyItv IC TC States EmptyStates Conds Trigs MaxDepth\n";
        str += QString::number(g.intervals) + " ";
        str += QString::number(g.empty_intervals) + " ";
        str += QString::number(g.events) + " ";
        str += QString::number(g.nodes) + " ";
        str += QString::number(g.states) + " ";
        str += QString::number(g.empty_states) + " ";
        str += QString::number(g.conditions) + " ";
        str += QString::number(g.triggers) + " ";
        str += QString::number(g.maxDepth) + " ";

        str += "\n\n";
        str += "Processes\n=======\n\n";
        str += "Procs ProcsPerItv ProcsPerLoadedItv Autom Mapping Scenar Loop Script Other\n";
        str += QString::number(g.processes) + " ";
        str += QString::number(g.processesPerInterval) + " ";
        str += QString::number(g.processesPerIntervalWithProcess) + " ";
        str += QString::number(g.automations + g.interpolations) + " ";
        str += QString::number(g.mappings) + " ";
        str += QString::number(g.scenarios) + " ";
        str += QString::number(g.loops) + " ";
        str += QString::number(g.scripts) + " ";
        str += QString::number(g.other) + " ";

        str += "\n\n";

        Scenario::TextDialog dial(str, qApp->activeWindow());
        dial.exec();
      });

  m_MLexport = new QAction{tr("To ML"), nullptr};
  connect(m_MLexport, &QAction::triggered, [&]() {
    auto doc = currentDocument();
    if (!doc)
      return;

    Scenario::ScenarioDocumentModel& base
        = score::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);
    auto& baseScenario = static_cast<Scenario::ProcessModel&>(
        *base.baseScenario().interval().processes.begin());

    using namespace stal::Metrics;
    // Language
    QString str = toML(baseScenario);
    Scenario::TextDialog dial(str, qApp->activeWindow());
    dial.exec();
  });
  m_CPPexport = new QAction{tr("To ossia"), nullptr};
  connect(m_CPPexport, &QAction::triggered, [&]() {
    auto doc = currentDocument();
    if (!doc)
      return;

    Scenario::ScenarioDocumentModel& base
        = score::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);
    auto& baseScenario = static_cast<Scenario::ProcessModel&>(
        *base.baseScenario().interval().processes.begin());

    using namespace stal::Metrics;
    // Language
    QString str = toCPP(baseScenario);
    Scenario::TextDialog dial(str, qApp->activeWindow());
    dial.exec();
  });
}

score::GUIElements stal::ApplicationPlugin::makeGUIElements()
{
  auto& m = context.menus.get().at(score::Menus::Export());
  QMenu* menu = m.menu();
  menu->addAction(m_MLexport);
  menu->addAction(m_CPPexport);
  menu->addAction(m_himito);
  menu->addAction(m_generate);
  menu->addAction(m_convert);
  menu->addAction(m_metrics);
  menu->addAction(m_TIKZexport);
  menu->addAction(m_statistics);

  return {};
}
