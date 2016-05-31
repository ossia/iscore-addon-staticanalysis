#pragma once
#include <iscore/plugins/qt_interfaces/PluginRequirements_QtInterface.hpp>
#include <iscore/plugins/qt_interfaces/GUIApplicationContextPlugin_QtInterface.hpp>
#include <QObject>

#include <iscore/plugins/application/GUIApplicationContextPlugin.hpp>

namespace iscore {

}  // namespace iscore

namespace TA
{
}
class iscore_addon_staticanalysis final:
        public QObject,
        public iscore::Plugin_QtInterface,
        public iscore::GUIApplicationContextPlugin_QtInterface
{
        Q_OBJECT
        Q_PLUGIN_METADATA(IID GUIApplicationContextPlugin_QtInterface_iid)
        Q_INTERFACES(
                iscore::Plugin_QtInterface
                iscore::GUIApplicationContextPlugin_QtInterface
                )

    public:
        iscore_addon_staticanalysis();
        virtual ~iscore_addon_staticanalysis() = default;

        iscore::GUIApplicationContextPlugin* make_applicationPlugin(
                const iscore::GUIApplicationContext& app) override;

        iscore::Version version() const override;
        UuidKey<iscore::Plugin> key() const override;

};
