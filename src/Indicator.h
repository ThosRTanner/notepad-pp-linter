#pragma once

#include <optional>
#include <stdint.h>
#include <unordered_map>

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
        Stroke_Width
    };

    typedef std::unordered_map<Property, uint64_t> Properties;

    bool colour_as_message() const noexcept;

    Properties const &properties() const noexcept
    {
        return properties_;
    }

    void read(std::optional<Dom_Node> node);

  private:
    Properties properties_;
};

}    // namespace Linter
