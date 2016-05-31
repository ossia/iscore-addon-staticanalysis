#include <StaticAnalysis/ScenarioVisitor.hpp>
#include "iscore_plugin_staticanalysis.hpp"

namespace iscore {

}  // namespace iscore

iscore_addon_staticanalysis::iscore_addon_staticanalysis() :
    QObject {}
{
}

iscore::GUIApplicationContextPlugin* iscore_addon_staticanalysis::make_applicationPlugin(
        const iscore::GUIApplicationContext& app)
{
    return new stal::ApplicationPlugin{app};
}

iscore::Version iscore_addon_staticanalysis::version() const
{
    return iscore::Version{1};
}

UuidKey<iscore::Plugin> iscore_addon_staticanalysis::key() const
{
    return "e1ef22f4-5fa3-4992-9f88-0e1ec5b5bb7f";
}
