#pragma once
#include <score/plugins/qt_interfaces/PluginRequirements_QtInterface.hpp>
#include <score/plugins/qt_interfaces/GUIApplicationPlugin_QtInterface.hpp>
#include <QObject>

#include <score/plugins/application/GUIApplicationPlugin.hpp>

class score_addon_staticanalysis final:
        public score::Plugin_QtInterface,
        public score::ApplicationPlugin_QtInterface
{
  SCORE_PLUGIN_METADATA(1, "e1ef22f4-5fa3-4992-9f88-0e1ec5b5bb7f")
    public:
        score_addon_staticanalysis();
        virtual ~score_addon_staticanalysis() = default;

        score::GUIApplicationPlugin* make_guiApplicationPlugin(
                const score::GUIApplicationContext& app) override;
};
