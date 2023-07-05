project(score_addon_staticanalysis LANGUAGES CXX)

if(NOT TARGET score_plugin_engine)
    return()
endif()
score_common_setup()

# Files & main target
set(HDRS
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/ScenarioMetrics.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/ScenarioVisitor.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/ScenarioGenerator.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/TAConversion.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/TIKZConversion.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/CppGenerator.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/ReactiveIS.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/score_addon_staticanalysis.hpp"
)
set(SRCS
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/ScenarioMetrics.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/ScenarioVisitor.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/ScenarioGenerator.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/Statistics.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/TAConversion.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/TIKZConversion.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/CppGenerator.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/ReactiveIS.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/score_addon_staticanalysis.cpp"
)

qt_add_resources(QRCS "${CMAKE_CURRENT_SOURCE_DIR}/StaticAnalysis/Resources/TAResources.qrc")
add_library(${PROJECT_NAME} ${SRCS} ${HDRS} ${QRCS})

target_link_libraries(${PROJECT_NAME} PUBLIC
    score_lib_base score_lib_process
    score_plugin_scenario score_plugin_automation
    score_plugin_engine score_plugin_js score_plugin_mapping)

setup_score_plugin(${PROJECT_NAME})
