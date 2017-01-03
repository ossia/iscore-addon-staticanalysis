#include <Scenario/Document/BaseScenario/BaseScenario.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
#include <Scenario/Document/State/StateModel.hpp>
#include <Scenario/Document/TimeNode/TimeNodeModel.hpp>
#include <Scenario/Document/TimeNode/Trigger/TriggerModel.hpp>
#include <Scenario/Process/ScenarioModel.hpp>

#include <iscore/actions/MenuManager.hpp>
#include <QAction>
#include <QChar>
#include <QDebug>

#include <QString>
#include <sstream>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Process/Process.hpp>
#include <Process/State/MessageNode.hpp>
#include <Scenario/Document/Constraint/ConstraintDurations.hpp>
#include <Scenario/Document/State/ItemModel/MessageItemModel.hpp>
#include <StaticAnalysis/ScenarioVisitor.hpp>
#include <State/Message.hpp>
#include <State/Value.hpp>

#include <iscore/document/DocumentInterface.hpp>
#include <iscore/plugins/application/GUIApplicationPlugin.hpp>
#include <iscore/model/Identifier.hpp>
#include <ossia/detail/algorithms.hpp>
#include <iscore/actions/Menu.hpp>
#include <core/document/Document.hpp>

#include <QApplication>
#include <QJsonDocument>
#include <QFile>
#include <QFileDialog>
#include <QSaveFile>
#include <StaticAnalysis/TAConversion.hpp>
#include <Scenario/Application/Menus/TextDialog.hpp>
#include <StaticAnalysis/ScenarioMetrics.hpp>
#include <StaticAnalysis/ScenarioGenerator.hpp>
#include <StaticAnalysis/TIKZConversion.hpp>
#include <iscore/actions/ActionManager.hpp>

stal::ApplicationPlugin::ApplicationPlugin(const iscore::GUIApplicationContext& app):
    iscore::GUIApplicationPlugin{app}
{
    m_himito = new QAction{tr("Generate scenario from Petri Net"), nullptr};
    connect(m_himito, &QAction::triggered, [&] () {

        auto doc = currentDocument();
        if(!doc)
            return;
        Scenario::ScenarioDocumentModel& base = iscore::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);

        const auto& baseConstraint = base.baseScenario().constraint();
        if(baseConstraint.processes.size() == 0)
            return;

        auto firstScenario = dynamic_cast<Scenario::ProcessModel*>(&*baseConstraint.processes.begin());
        if(!firstScenario)
            return;

        CommandDispatcher<> disp(doc->context().commandStack);

        stal::generateScenarioFromPetriNet(*firstScenario, disp);
    });
    m_generate = new QAction{tr("Generate random score"), nullptr};
    connect(m_generate, &QAction::triggered, [&] () {
        auto doc = currentDocument();
        if(!doc)
            return;
        Scenario::ScenarioDocumentModel& base = iscore::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);

        const auto& baseConstraint = base.baseScenario().constraint();
        if(baseConstraint.processes.size() == 0)
            return;

        auto firstScenario = dynamic_cast<Scenario::ProcessModel*>(&*baseConstraint.processes.begin());
        if(!firstScenario)
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

                QString baseName = "data/" + QString::number(set) + "/" + QString::number(n);
                {
                    QFile savefile(baseName + ".scorejson");
                    savefile.open(QFile::WriteOnly);
                    QJsonDocument jdoc(doc->saveAsJson());

                    savefile.write(jdoc.toJson());
                    savefile.close();
                }

                {
                    QString text = TA::makeScenario(base.baseScenario().constraint());
                    QFile f(baseName + ".xml");
                    f.open(QFile::WriteOnly);
                    f.write(text.toUtf8());
                    f.close();
                }
            }
        }
            */
        stal::generateScenario(*firstScenario, 300, disp);

    });
    m_convert = new QAction{tr("Convert to Temporal Automatas"), nullptr};
    connect(m_convert, &QAction::triggered, [&] () {
        auto doc = currentDocument();
        if(!doc)
            return;
        Scenario::ScenarioDocumentModel& base = iscore::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);

        QString text = TA::makeScenario(base.baseScenario().constraint());
        Scenario::TextDialog dial(text, qApp->activeWindow());
        dial.exec();

        QFile f("model-output.xml");
        f.open(QFile::WriteOnly);
        f.write(text.toUtf8());
        f.close();
    } );

    m_metrics = new QAction{tr("Scenario metrics"), nullptr};
    connect(m_metrics, &QAction::triggered, [&] () {

        auto doc = currentDocument();
        if(!doc)
            return;

        Scenario::ScenarioDocumentModel& base = iscore::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);
        auto& baseScenario = static_cast<Scenario::ProcessModel&>(*base.baseScenario().constraint().processes.begin());

        using namespace stal::Metrics;
        // Language
        QString str = toScenarioLanguage(baseScenario);

        // Halstead
        {
            auto factors = Halstead::ComputeFactors(baseScenario);
            str += "Difficulty = " + QString::number(Halstead::Difficulty(factors)) + "\n";
            str += "Volume = " + QString::number(Halstead::Volume(factors)) + "\n";
            str += "Effort = " + QString::number(Halstead::Effort(factors)) + "\n";
            str += "TimeRequired = " + QString::number(Halstead::TimeRequired(factors)) + "\n";
            str += "Bugs2 = " + QString::number(Halstead::Bugs2(factors)) + "\n";
        }
        // Cyclomatic
        {
            auto factors = Cyclomatic::ComputeFactors(baseScenario);
            str += "Cyclomatic1 = " + QString::number(Cyclomatic::Complexity(factors));
        }
        // Display
        Scenario::TextDialog dial(str, qApp->activeWindow());
        dial.exec();
    });

    m_TIKZexport = new QAction{tr("Export in TIKZ"), nullptr};
    connect(m_TIKZexport, &QAction::triggered, [&] () {
        auto doc = currentDocument();
        if(!doc)
            return;
        Scenario::ScenarioDocumentModel& base = iscore::IDocument::get<Scenario::ScenarioDocumentModel>(*doc);
        auto& baseScenario = static_cast<Scenario::ProcessModel&>(*base.baseScenario().constraint().processes.begin());

        QFileDialog d{qApp->activeWindow(), tr("Save Document As")};
        d.setNameFilter(("tex files (*.tex)"));
        d.setConfirmOverwrite(true);
        d.setFileMode(QFileDialog::AnyFile);
        d.setAcceptMode(QFileDialog::AcceptSave);

        if(d.exec())
        {
            auto files = d.selectedFiles();
            QString savename = files.first();
            QString name = savename;
            name.remove(0, savename.lastIndexOf("/") + 1);
            name.remove(".tex");
            if(!savename.isEmpty())
            {
                if(!savename.contains(".tex"))
                    savename += ".tex";

                QSaveFile f{savename};
                f.open(QIODevice::WriteOnly);

                QString tex = makeTIKZ(name, baseScenario);
                f.write(tex.toUtf8());
                f.commit();
            }
        }

    });
}

iscore::GUIElements stal::ApplicationPlugin::makeGUIElements()
{
    auto& m = context.menus.get().at(iscore::Menus::Export());
    QMenu* menu = m.menu();
    menu->addAction(m_himito);
    menu->addAction(m_generate);
    menu->addAction(m_convert);
    menu->addAction(m_metrics);
    menu->addAction(m_TIKZexport);

    return {};
}
