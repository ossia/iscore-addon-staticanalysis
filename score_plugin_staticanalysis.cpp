#include <StaticAnalysis/ScenarioVisitor.hpp>
#include "score_plugin_staticanalysis.hpp"

namespace score {

}  // namespace score

score_addon_staticanalysis::score_addon_staticanalysis() :
    QObject {}
{
}

score::GUIApplicationPlugin* score_addon_staticanalysis::make_guiApplicationPlugin(
        const score::GUIApplicationContext& app)
{
    return new stal::ApplicationPlugin{app};
}
