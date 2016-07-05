#pragma once

#include <QString>
#include <Scenario/Process/ScenarioModel.hpp>
#include <Scenario/Document/Constraint/ConstraintModel.hpp>
#include <Scenario/Document/TimeNode/TimeNodeModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>

QString makeTIKZ(QString name, Scenario::ProcessModel& scenario);
QString makeTIKZ(QString name, Scenario::ProcessModel& scenario)
{
    int HEIGHT = 75;
    QString texString;

    texString += "\\def\\schemaScenario " + name + "{%\n";
    texString += "\\begin{tikzpicture}[scale=0.3]%\n";

    for(auto &node : scenario.timeNodes)
    {
	if(node.id() == scenario.endEvent().timeNode())
	    continue;

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
