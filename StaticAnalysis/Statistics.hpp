#pragma once
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Scenario/Process/ScenarioInterface.hpp>

namespace stal
{
struct ScenarioStatistics
{
    int constraints{};
    int events{};
    int nodes{};
    int empty_states{};
    int states{};

    int processes{};
    int processesPerConstraint{};
    int processesPerConstraintWithProcess{};

    int automations{};
    int mappings{};
    int scenarios{};
    int loops{};

    ScenarioStatistics(const Scenario::ScenarioInterface& scenar)
    {

    }
};

struct DeviceStatistics
{
    QString name;
    int nodes{};
    int non_leaf_nodes{};
    int max_depth{};
    int max_child_count{}; // not recursive, for a single node
    double avg_child_count{};
    double avg_non_leaf_child_count{}; // for nodes that have > 0 children

    int containers{};
    int int_addr{};
    int impulse_addr{};
    int float_addr{};
    int bool_addr{};
    int vec2f_addr{};
    int vec3f_addr{};
    int vec4f_addr{};
    int tuple_addr{};
    int string_addr{};
    int char_addr{};

    int num_get{};
    int num_set{};
    int num_bi{};

    double avg_ext_metadata{};



    int cur_depth{};

    template <typename Fun>
    void visit(Fun f, const Device::Node& node)
    {
        f(node);

        cur_depth ++;
        if(cur_depth > max_depth)
            max_depth = cur_depth;

        for (const auto& child : node.children())
        {
            visit(f, child);
        }
        cur_depth--;
    }

    DeviceStatistics(const Device::Node& node)
    {
        name = node.displayName();

        for (const auto& child : node.children())
        {
            visit([&] (const Device::Node& child) {
                nodes++;
                const auto N = child.childCount();
                if(N > 0)
                    non_leaf_nodes++;
                if(N > max_child_count)
                    max_child_count = child.childCount();
                avg_child_count += N;
                avg_non_leaf_child_count += N;

                const Device::AddressSettings& addr = child.get<Device::AddressSettings>();
                if(addr.value.valid())
                {
                    switch(addr.value.getType())
                    {
                    case ossia::val_type::NONE: break;
                    case ossia::val_type::INT: int_addr++; break;
                    case ossia::val_type::IMPULSE: impulse_addr++; break;
                    case ossia::val_type::FLOAT: float_addr++; break;
                    case ossia::val_type::BOOL: bool_addr++; break;
                    case ossia::val_type::VEC2F: vec2f_addr++; break;
                    case ossia::val_type::VEC3F: vec3f_addr++; break;
                    case ossia::val_type::VEC4F: vec4f_addr++; break;
                    case ossia::val_type::TUPLE: tuple_addr++; break;
                    case ossia::val_type::CHAR: char_addr++; break;
                    case ossia::val_type::STRING: string_addr++; break;
                    }

                    avg_ext_metadata += addr.extendedAttributes.size();

                    ISCORE_ASSERT(addr.ioType);
                    {
                        switch(*addr.ioType)
                        {
                        case ossia::access_mode::SET: num_set++; break;
                        case ossia::access_mode::GET: num_get++; break;
                        case ossia::access_mode::BI: num_bi++; break;
                        }

                    }
                }
                else
                {
                    containers++;
                }


            }, child);
        }

        avg_child_count /= nodes;
        avg_non_leaf_child_count /= non_leaf_nodes;
        avg_ext_metadata /= nodes;
    }
};

struct ExplorerStatistics
{
    std::vector<DeviceStatistics> devices;

    ExplorerStatistics(const Explorer::DeviceDocumentPlugin& doc)
    {
        auto& root = doc.rootNode();
        for(auto& cld : root.children())
        {
            devices.emplace_back(cld);
        }
    }
};
}
