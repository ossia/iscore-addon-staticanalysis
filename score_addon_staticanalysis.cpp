#include "score_addon_staticanalysis.hpp"

#include <core/application/ApplicationSettings.hpp>

#include <StaticAnalysis/ScenarioVisitor.hpp>

score_addon_staticanalysis::score_addon_staticanalysis()
{
#if defined(SCORE_STATIC_PLUGINS)
  Q_INIT_RESOURCE(TAResources);
#endif
}

score::GUIApplicationPlugin*
score_addon_staticanalysis::make_guiApplicationPlugin(
    const score::GUIApplicationContext& app)
{
  if(!app.applicationSettings.gui)
    return nullptr;
  return new stal::ApplicationPlugin{app};
}

#include <score/plugins/PluginInstances.hpp>
SCORE_EXPORT_PLUGIN(score_addon_staticanalysis)
