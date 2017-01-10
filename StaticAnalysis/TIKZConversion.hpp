#pragma once
#include <QString>

namespace Scenario
{
class ProcessModel;
}

namespace stal
{
QString makeTIKZ(QString name, Scenario::ProcessModel& scenario);
QString makeTIKZ2(QString name, Scenario::ProcessModel& scenario);
}
