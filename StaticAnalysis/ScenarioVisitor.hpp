#pragma once
#include <score/plugins/application/GUIApplicationPlugin.hpp>

class QAction;
namespace score {

class MenubarManager;
}  // namespace score
// RENAMEME
namespace stal
{
class ApplicationPlugin :
        public QObject,
        public score::GUIApplicationPlugin
{
    public:
        ApplicationPlugin(const score::GUIApplicationContext& app);

    private:
        score::GUIElements makeGUIElements() override;

        QAction* m_himito{};
        QAction* m_generate{};
        QAction* m_convert{};
        QAction* m_metrics{};
        QAction* m_MLexport{};
        QAction* m_TIKZexport{};
        QAction* m_statistics{};

};
}
