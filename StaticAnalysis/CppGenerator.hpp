#pragma once
#include <QString>
namespace Scenario
{
class ProcessModel;
}
namespace stal
{
QString toCPP(const Scenario::ProcessModel& s);
}
