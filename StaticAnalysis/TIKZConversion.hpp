#pragma once

#include <QString>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/TimeNode/TimeNodeModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <State/Expression.hpp>

#include <QStringBuilder>
QString makeTIKZ(QString name, Scenario::ProcessModel& scenario);
QString makeTIKZ(QString name, Scenario::ProcessModel& scenario)
{
  int HEIGHT = 75;
  QString texString;

  texString += "\\def\\schemaScenario " + name + "{%\n";
  texString += "\\begin{tikzpicture}[scale=0.3]%\n";

  for(auto &node : scenario.timeNodes)
  {

    int date = (int) node.date().sec();
    texString += "\\def\\date{" + QString::number(date) + "};%\n";

    auto nodeName = "\\Huge T"+ QString::number(node.id_val());

    texString += "\\def\\nodeName{" + nodeName + "};%\n";
    texString += "\\def\\bottom{" + QString::number(-node.extent().bottom() * HEIGHT) + "};%\n";
    texString += "\\def\\top{" + QString::number(-node.extent().top() * HEIGHT) + "};%\n";
    texString += "\\draw (\\date,\\top) -- ++(-1,0.5) -- ++(0,2) -- ++(2,0) -- ++(0,-2) -- ++(-1,-0.5) -- (\\date, \\bottom);%\n";
    texString += "\\draw (\\date,\\top + 1.5) node[scale=0.5]{\\nodeName};%\n%\n";
  }

  for(auto &constraint : scenario.constraints)
  {
    QString constraintName = "C"+QString::number(constraint.id_val());
    auto& endTn = Scenario::endTimeNode(constraint, scenario);

    auto yPos = -constraint.heightPercentage() * HEIGHT;
    auto date = (int) constraint.startDate().sec();
    auto nom = int(endTn.date().sec()) - date;
    int min = nom;
    if(constraint.duration.minDuration() != constraint.duration.defaultDuration())
      min = (int)constraint.duration.minDuration().sec();

    auto max = nom;
    if(constraint.duration.maxDuration() != constraint.duration.defaultDuration())
      max = constraint.duration.isMaxInfinite()
          ? (int)constraint.duration.defaultDuration().sec()+1
          : (int)constraint.duration.maxDuration().sec();



    texString += "\\def\\contraintName{" + constraintName + "};%\n";
    texString += "\\def\\ypos{" + QString::number(yPos) + "};%\n";
    texString += "\\def\\date{" + QString::number(date) + "};\n";
    texString += "\\def\\min{" + QString::number(min) + "};%\n";
    texString += "\\def\\nom{" + QString::number(nom) + "};%\n";
    texString += "\\def\\max{" + QString::number(max) + "};%\n";

    texString += "\\fill (\\date,\\ypos) circle (0.3);%\n";
    texString += "\\fill (\\date+\\nom,\\ypos) circle (0.3);%\n";
    texString += "\\draw (\\date,\\ypos) -- ++(\\min,0) node[midway,above,scale=1] {\\contraintName};%\n";
    texString += "\\draw (\\date+\\min,\\ypos+0.5) -- ++(0,-1);%\n";
    texString += "\\draw[dotted] (\\date+\\min,\\ypos) -- (\\date+\\max,\\ypos);%\n";
    if(!constraint.duration.isMaxInfinite())
    {texString += "\\draw (\\date+\\max,\\ypos+0.5) -- ++(0,-1);%\n%\n";}
  }
  texString += "\\end{tikzpicture}%\n";
  texString += "}\n";

  return texString;
}


QString makeTIKZ2(QString name, Scenario::ProcessModel& scenario);
QString makeTIKZ2(QString name, Scenario::ProcessModel& scenario)
{
  // First compute distance between lowest and highest state
  double min_y = 100, max_y = 0;
  double max_x = 0;
  for(const Scenario::StateModel& state : scenario.states)
  {
    const auto h = 1. - state.heightPercentage();
    if(h < min_y) min_y = h;
    if(h > max_y) max_y = h;

    auto& tn = Scenario::parentTimeNode(state, scenario);
    const auto t = tn.date().msec();
    if(t > max_x)
      max_x = t;
  }
  const double height = 200. * (max_y - min_y);
  const double width = 5. / max_x;

  const auto fill = QStringLiteral("\\fill ");
  const auto draw = QStringLiteral("\\draw ");
  const auto draw_full = QStringLiteral("\\draw[line width=1pt] ");
  const auto draw_dash = QStringLiteral("\\draw[dashed,line width=1pt] ");
  const auto circle = QStringLiteral("circle ");
  const auto num = [] (double x) { return QString{"(%1) "}.arg(x); };
  const auto point = [] (double x, double y) { return QString{"(%1, %2) "}.arg(x).arg(y); };

  const auto fin = QStringLiteral(";\n");
  const auto endname = [] (const auto& t) { return QString{"; % %1 \n"}.arg(t.metadata().getName()); };
  const auto makelabel = [=] (double x, double y, const QString& lab) -> QString {
    return draw % point(x, y) % "node {$" % lab % "$}" ;
  };

  const auto draw_arc = QStringLiteral("\\draw[line width=0.7pt] ");
  const auto cstMin = QStringLiteral("arc(90:270:0.15) ");
  const auto cstMax = QStringLiteral("arc(-90:90:0.15) ");

  QString texString;
  texString.reserve(10000);

  using namespace Scenario;
  for(const StateModel& state : scenario.states)
  {
    auto& tn = Scenario::parentTimeNode(state, scenario);
    const auto x = tn.date().msec();
    const auto y = 1. - state.heightPercentage();
    texString += fill % point(x * width, y * height) % circle % num(0.075) % endname(state);
  }

  for(const TimeNodeModel& node : scenario.timeNodes)
  {
    const auto x = node.date().msec();
    const auto y = node.extent();

    texString += draw_full % point(x * width, (1. - y.top()) * height) % " -- " % point(x * width, (1. - y.bottom()) * height) % endname(node);

    const auto& label = node.metadata().getLabel();
    if(!label.isEmpty())
    {
      texString += makelabel(x * width, (1. - y.top()) * height + 0.25, label) % endname(node);
    }
  }

  for(const ConstraintModel& cst : scenario.constraints)
  {
    const auto x0 = cst.startDate().msec();
    const auto xDef = x0 + cst.duration.defaultDuration().msec();
    const auto xMid = x0 + cst.duration.defaultDuration().msec() / 2.;
    const auto xMin = x0 + cst.duration.minDuration().msec();
    const auto xMax = x0 + cst.duration.maxDuration().msec();
    const bool rigid = cst.duration.isRigid();
    const bool minNull = cst.duration.isMinNul();
    const bool maxInf = cst.duration.isMaxInfinite();
    const auto y = 1. - cst.heightPercentage();

    if(rigid)
    {
      texString += draw_full % point(x0 * width, y * height) % " -- " % point(xDef * width, y * height) % endname(cst);
    }
    else if(maxInf)
    {
      if(minNull)
      {
        texString += draw_dash % point(x0 * width, y * height) % " -- " % point(xDef * width, y * height) % endname(cst);
      }
      else
      {
        texString += draw_full % point(x0 * width, y * height) % " -- " % point(xMin * width, y * height) % endname(cst);
        texString += draw_dash % point(xMin * width, y * height) % " -- " % point(xDef * width, y * height) % endname(cst);
        texString += draw_arc % point(xMin * width + 0.24, y * height + 0.153) % cstMin % endname(cst);
      }
    }
    else
    {
      if(minNull)
      {
        texString += draw_dash % point(x0 * width, y * height) % " -- " % point(xMax * width, y * height) % endname(cst);
      }
      else
      {
        texString += draw_full % point(x0 * width, y * height) % " -- " % point(xMin * width, y * height) % endname(cst);
        texString += draw_dash % point(xMin * width, y * height) % " -- " % point(xMax * width, y * height) % endname(cst);
        texString += draw_arc % point(xMin * width + 0.24, y * height + 0.153) % cstMin % endname(cst);
        texString += draw_arc % point(xMax * width - 0.19, y * height - 0.15) % cstMax % endname(cst);
      }
    }

    const auto& label = cst.metadata().getLabel();
    if(!label.isEmpty())
    {
      texString += makelabel(xMid * width, y * height + 0.2, label) % endname(cst);
    }
  }

  for(const EventModel& ev: scenario.events)
  {
    if(!State::isTrueExpression(ev.condition().toString()))
    {
      auto y = ev.extent();
      auto x = parentTimeNode(ev, scenario).date().msec();

      texString += draw_full % point(x * width - 0.2, (1. - y.top()) * height) % " -- " % point(x * width - 0.2, (1. - y.bottom()) * height) % endname(ev);
      texString += draw_full % point(x * width - 0.2, (1. - y.top()) * height) % "arc(180:75:0.2) " % endname(ev);
      texString += draw_full % point(x * width - 0.2, (1. - y.bottom()) * height) % "arc(180:285:0.2) " % endname(ev);
    }
  }
  return texString;
}
