#pragma once
#include <score/plugins/qt_interfaces/PluginRequirements_QtInterface.hpp>
#include <score/plugins/qt_interfaces/GUIApplicationPlugin_QtInterface.hpp>
#include <QObject>

#include <score/plugins/application/GUIApplicationPlugin.hpp>

namespace score {

}  // namespace score

namespace TA
{
}
class score_addon_staticanalysis final:
        public QObject,
        public score::Plugin_QtInterface,
        public score::ApplicationPlugin_QtInterface
{
        Q_OBJECT
        Q_PLUGIN_METADATA(IID ApplicationPlugin_QtInterface_iid)
        Q_INTERFACES(
                score::Plugin_QtInterface
                score::ApplicationPlugin_QtInterface
                )

  SCORE_PLUGIN_METADATA(1, "e1ef22f4-5fa3-4992-9f88-0e1ec5b5bb7f")
    public:
        score_addon_staticanalysis();
        virtual ~score_addon_staticanalysis() = default;

        score::GUIApplicationPlugin* make_guiApplicationPlugin(
                const score::GUIApplicationContext& app) override;
};
