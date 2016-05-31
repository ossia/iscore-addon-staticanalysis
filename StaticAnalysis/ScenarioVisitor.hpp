#pragma once
#include <iscore/plugins/application/GUIApplicationContextPlugin.hpp>

class QAction;
namespace iscore {

class MenubarManager;
}  // namespace iscore
// RENAMEME
namespace stal
{
class ApplicationPlugin :
        public QObject,
        public iscore::GUIApplicationContextPlugin
{
    public:
        ApplicationPlugin(const iscore::GUIApplicationContext& app);

    private:
        iscore::GUIElements makeGUIElements() override;

        QAction* m_himito{};
        QAction* m_generate{};
        QAction* m_convert{};
        QAction* m_metrics{};
        QAction* m_TIKZexport{};

};
}
