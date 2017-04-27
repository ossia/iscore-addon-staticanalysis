#include "TIKZConversion.hpp"
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/TimeNode/TimeNodeModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <State/Expression.hpp>

#include <QStringBuilder>

#include <Automation/AutomationModel.hpp>

namespace stal
{
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



static const auto fill = QStringLiteral("\\fill ");
static const auto draw = QStringLiteral("\\draw ");
static const auto draw_full = QStringLiteral("\\draw[line width=1pt] ");
static const auto draw_dash = QStringLiteral("\\draw[dashed,line width=1pt] ");
static const auto circle = QStringLiteral("circle ");
static const auto num = [] (double x) { return QString{"(%1) "}.arg(x); };
static const auto fin = QStringLiteral(";\n");

static const auto draw_arc = QStringLiteral("\\draw[line width=0.7pt] ");
static const auto cstMin = QStringLiteral("arc(90:270:0.15) ");
static const auto cstMax = QStringLiteral("arc(-90:90:0.15) ");


struct TIKZVisitor
{
  static auto point(double x, double y)
  { return QString{"(%1, %2) "}.arg(x).arg(y); }
  static auto point(QPointF p)
  { return point(p.x(), p.y()); }

  template<typename T>
  static auto endname(const T& t)
  { return QString{"; % %1 \n"}.arg(t.metadata().getName()); }

  static QString makelabel(double x, double y, const QString& lab)
  { return draw % point(x, y) % "node {$" % lab % "$}"; }

  QString tikz;

  TIKZVisitor()
  {
    tikz.reserve(10000);
  }

  QRectF rootRect(Scenario::ProcessModel& scenario) const
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

    return QRectF(0, 0, width, height);
  }

  void operator()(Scenario::ProcessModel& scenario, optional<QRectF> root)
  {
    using namespace Scenario;

    QRectF r = root ? *root : rootRect(scenario);
    qDebug() << r ;

    for(const StateModel& state : scenario.states)
    {
      (*this)(state, scenario, r);
    }

    for(const TimeNodeModel& node : scenario.timeNodes)
    {
      (*this)(node, r);
    }

    for(const ConstraintModel& cst : scenario.constraints)
    {
      (*this)(cst, r);
    }

    for(const EventModel& ev: scenario.events)
    {
      (*this)(ev, scenario, r);
    }
  }

  void operator()(const Scenario::StateModel& state, const Scenario::ProcessModel& scenario, QRectF r)
  {
    auto& tn = Scenario::parentTimeNode(state, scenario);
    const auto x = tn.date().msec();
    const auto y = 1. - state.heightPercentage();
    tikz += fill
            % point(r.x() + x * r.width(), -r.y() + y * r.height())
            % circle % num(0.075) % endname(state);

    const auto& label = state.metadata().getLabel();
    if(!label.isEmpty())
    {
      tikz += makelabel(
                 r.x() + x * r.width() + 0.15,
                -r.y() + (1. - y) * r.height() + 0.25,
                label) % endname(state);
    }
  }

  void operator()(const Scenario::TimeNodeModel& node, QRectF r)
  {
    const auto x = node.date().msec();
    const auto y = node.extent();

    tikz += draw_full
            % point(r.x() + x * r.width(), -r.y() + (1. - y.top()) * r.height()) % " -- "
            % point(r.x() + x * r.width(), -r.y() + (1. - y.bottom()) * r.height())
            % endname(node);

    const auto& label = node.metadata().getLabel();
    if(!label.isEmpty())
    {
      tikz += makelabel(
                 r.x() + x * r.width(),
                -r.y() + (1. - y.top()) * r.height() + 0.25,
                label) % endname(node);
    }
  }

  void operator()(const Scenario::ConstraintModel& cst, QRectF r)
  {
    // Required data
    const auto x0 = cst.startDate().msec();
    const auto xDef = x0 + cst.duration.defaultDuration().msec();
    const auto xMid = x0 + cst.duration.defaultDuration().msec() / 2.;
    const auto xMin = x0 + cst.duration.minDuration().msec();
    const auto xMax = x0 + cst.duration.maxDuration().msec();
    const bool rigid = cst.duration.isRigid();
    const bool minNull = cst.duration.isMinNul();
    const bool maxInf = cst.duration.isMaxInfinite();
    const auto y = 1. - cst.heightPercentage();

    const QPointF tikz_origin{r.x() + x0 * r.width(), -r.y() + y * r.height()};
    const QPointF tikz_default{r.x() + xDef * r.width(),  -r.y() + y * r.height()};
    const QPointF tikz_min{r.x() + xMin * r.width(),  -r.y() + y * r.height()};
    const QPointF tikz_max{r.x() + xMax * r.width(),  -r.y() + y * r.height()};
    const QPointF tikz_minArc{r.x() + xMin * r.width() + 0.24, -r.y() + y * r.height() + 0.153};
    const QPointF tikz_maxArc{r.x() + xMax * r.width() - 0.15, -r.y() + y * r.height() - 0.15};
    // Drawing the constraint
    if(rigid)
    {
      tikz += draw_full % point(tikz_origin) % " -- " % point(tikz_default) % endname(cst);
    }
    else if(maxInf)
    {
      if(minNull)
      {
        tikz += draw_dash % point(tikz_origin) % " -- " % point(tikz_default) % endname(cst);
      }
      else
      {
        tikz += draw_full % point(tikz_origin) % " -- " % point(tikz_min) % endname(cst);
        tikz += draw_dash % point(tikz_min) % " -- " % point(tikz_default) % endname(cst);
        tikz += draw_arc % point(tikz_minArc) % cstMin % endname(cst);
      }
    }
    else
    {
      if(minNull)
      {
        tikz += draw_dash % point(tikz_origin) % " -- " % point(tikz_max) % endname(cst);
        tikz += draw_arc % point(tikz_maxArc) % cstMax % endname(cst);
      }
      else
      {
        tikz += draw_full % point(tikz_origin) % " -- " % point(tikz_min) % endname(cst);
        tikz += draw_dash % point(tikz_min) % " -- " % point(tikz_max) % endname(cst);
        tikz += draw_arc % point(tikz_minArc) % cstMin % endname(cst);
        tikz += draw_arc % point(tikz_maxArc) % cstMax % endname(cst);
      }
    }

    // Drawing the label
    const auto& label = cst.metadata().getLabel();
    if(!label.isEmpty())
    {
      tikz += makelabel(
                 r.x() + xMid * r.width(),
                -r.y() + y * r.height() + 0.2, label) % endname(cst);
    }

    // Drawing the processes

    int pos = 0;
    QRectF proc_rect = r;
    proc_rect.setX(tikz_origin.x());
    proc_rect.setY(tikz_origin.y() - 0.1);
    proc_rect.setWidth(tikz_default.x() - tikz_origin.x());
    proc_rect.setHeight(1);

    for(auto& process : cst.processes)
    {
      tikz += draw_full
              % point(proc_rect.topLeft()) % " -- "
              % point(proc_rect.topRight()) % " -- "
              % point(bottomRight(proc_rect)) % " -- "
              % point(bottomLeft(proc_rect)) % " -- "
              % point(proc_rect.topLeft()) % fin;

      if(auto autom = dynamic_cast<Automation::ProcessModel*>(&process))
      {
        (*this)(*autom, proc_rect);
        pos++;
      }
      else if(auto scenar = dynamic_cast<Scenario::ProcessModel*>(&process))
      {
        tikz += makelabel(
              proc_rect.center().x(),
               proc_rect.y() - proc_rect.height() / 2.,
              scenar->metadata().getName())
            % endname(*scenar);
      }
    }
  }

  // tikz coordinates are reversed wrt Qt
  static QPointF bottomLeft(QRectF r)
  { return {r.topLeft().x(), r.topLeft().y() - r.height()}; }
  static QPointF bottomRight(QRectF r)
  { return {r.topRight().x(), r.topRight().y() - r.height()}; }

  void operator()(const Automation::ProcessModel& state, QRectF r)
  {
    tikz += draw_full
            % point(bottomLeft(r)) % " -- "
            % point(r.topRight())
            % fin;
  }


  void operator()(const Scenario::EventModel& ev, const Scenario::ProcessModel& scenario, QRectF r)
  {
    if(!State::isTrueExpression(ev.condition().toString()))
    {
      auto y = ev.extent();
      auto x = parentTimeNode(ev, scenario).date().msec();

      double ev_x = r.x() + x * r.width() - 0.2;
      double ev_top = -r.y() + (1. - y.top()) * r.height();
      double ev_bottom = -r.y() + (1. - y.bottom()) * r.height();
      tikz += draw_full
              % point(ev_x, ev_top) % " -- "
              % point(ev_x, ev_bottom) % endname(ev);

      tikz += draw_full
              % point(ev_x, ev_top)
              % "arc(180:75:0.2) " % endname(ev);

      tikz += draw_full
              % point(ev_x, ev_bottom)
              % "arc(180:285:0.2) " % endname(ev);
    }
  }
};

QString makeTIKZ2(QString name, Scenario::ProcessModel& scenario)
{
  TIKZVisitor v;
  v(scenario, ossia::none);
  return v.tikz;
}

}
