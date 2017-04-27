#pragma once
#include <iscore/plugins/qt_interfaces/PluginRequirements_QtInterface.hpp>
#include <iscore/plugins/qt_interfaces/GUIApplicationPlugin_QtInterface.hpp>
#include <QObject>

#include <iscore/plugins/application/GUIApplicationPlugin.hpp>

namespace iscore {

}  // namespace iscore

namespace TA
{
}
class iscore_addon_staticanalysis final:
        public QObject,
        public iscore::Plugin_QtInterface,
        public iscore::ApplicationPlugin_QtInterface
{
        Q_OBJECT
        Q_PLUGIN_METADATA(IID ApplicationPlugin_QtInterface_iid)
        Q_INTERFACES(
                iscore::Plugin_QtInterface
                iscore::ApplicationPlugin_QtInterface
                )

  ISCORE_PLUGIN_METADATA(1, "e1ef22f4-5fa3-4992-9f88-0e1ec5b5bb7f")
    public:
        iscore_addon_staticanalysis();
        virtual ~iscore_addon_staticanalysis() = default;

        iscore::GUIApplicationPlugin* make_guiApplicationPlugin(
                const iscore::GUIApplicationContext& app) override;
};
