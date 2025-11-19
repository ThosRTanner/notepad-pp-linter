#pragma once

#include <optional>
#include <unordered_map>

#include <stdint.h>

namespace Linter
{

class Dom_Node;

class Indicator
{
  public:
    enum Property
    {
        Style,
        Colour,
        Dynamic_Colour,
        Opacity,
        Outline_Opacity,
        Draw_Under,
        Stroke_Width,
        Hover_Style,
        Hover_Colour,
    };

    using Properties = std::unordered_map<Property, uint32_t>;

    bool colour_as_message() const noexcept;

    Properties const &properties() const noexcept
    {
        return properties_;
    }

    void read_config(std::optional<Dom_Node> node);

  private:
    Properties properties_;
};

}    // namespace Linter
