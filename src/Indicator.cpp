#include "Indicator.h"

#include "Dom_Node.h"
#include "Settings.h"

#include <plugintemplate/src/Scintilla.h>

#include <algorithm>
#include <cwctype>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace Linter
{

namespace
{
std::wstring to_lower(std::wstring_view data)
{
    std::wstring res;
    std::transform(
        data.begin(),
        data.end(),
        std::back_inserter(res),
        [](wchar_t c) noexcept { return std::towlower(c); }
    );
    return res;
}
}    // namespace

bool Indicator::colour_as_message() const noexcept
{
#pragma warning(suppress : 26447)
    return properties_.at(Dynamic_Colour);
}

void Indicator::read(std::optional<Dom_Node> node)
{
    // Default values
    properties_[Indicator::Style] = INDIC_FULLBOX;
    properties_[Indicator::Colour] = 0x0000ff;     // Red
    properties_[Indicator::Dynamic_Colour] = 0;    // False
    properties_[Indicator::Opacity] = 30;
    properties_[Indicator::Outline_Opacity] = 50;
    properties_[Indicator::Draw_Under] = 0;
    properties_[Indicator::Stroke_Width] = 100;

    if (not node.has_value())
    {
        return;
    }

    // A note - this is fairly dodgy, but as all the strings involved are
    // pure ascii it's safe.
#define STR_TO_INDIC(STR)            \
    {                                \
        to_lower(L#STR), INDIC_##STR \
    }
    static std::unordered_map<std::wstring, int> const styles = {
        STR_TO_INDIC(BOX),
        STR_TO_INDIC(COMPOSITIONTHICK),
        STR_TO_INDIC(COMPOSITIONTHIN),
        STR_TO_INDIC(DASH),
        STR_TO_INDIC(DIAGONAL),
        STR_TO_INDIC(DOTBOX),
        STR_TO_INDIC(DOTS),
        STR_TO_INDIC(EXPLORERLINK),
        STR_TO_INDIC(FULLBOX),
        STR_TO_INDIC(GRADIENT),
        STR_TO_INDIC(GRADIENTCENTRE),
        STR_TO_INDIC(HIDDEN),
        STR_TO_INDIC(PLAIN),
        STR_TO_INDIC(POINT),
        STR_TO_INDIC(POINT_TOP),
        STR_TO_INDIC(POINTCHARACTER),
        STR_TO_INDIC(ROUNDBOX),
        STR_TO_INDIC(SQUIGGLE),
        STR_TO_INDIC(SQUIGGLELOW),
        STR_TO_INDIC(SQUIGGLEPIXMAP),
        STR_TO_INDIC(STRAIGHTBOX),
        STR_TO_INDIC(STRIKE),
        STR_TO_INDIC(TEXTFORE),
        STR_TO_INDIC(TT),
    };
#undef STR_TO_INDIC

    if (auto const style = node->get_optional_node("./style");
        style.has_value())
    {
        properties_[Indicator::Style] = styles.at(style->get_value());
    }

    if (auto const shade = node->get_optional_node("./colour/shade");
        shade.has_value())
    {
        properties_[Indicator::Colour] = Settings::read_colour_node(*shade);
    }

    if (auto const as_message = node->get_optional_node("./colour/as_message");
        as_message.has_value())
    {
        properties_[Indicator::Dynamic_Colour] = SC_INDICFLAG_VALUEFORE;
    }

    if (auto const fill_opacity = node->get_optional_node("./fill_opacity");
        fill_opacity.has_value())
    {
        properties_[Indicator::Opacity] = std::stoul(fill_opacity->get_value());
    }

    if (auto const outline_opacity =
            node->get_optional_node("./outline_opacity");
        outline_opacity.has_value())
    {
        properties_[Indicator::Outline_Opacity] =
            std::stoul(outline_opacity->get_value());
    }

    if (auto const draw_under = node->get_optional_node("./draw_under");
        draw_under.has_value())
    {
        properties_[Indicator::Draw_Under] = 1;
    }

    if (auto const stroke_width = node->get_optional_node("./stroke_width");
        stroke_width.has_value())
    {
        // This is not ideal, but C++ doesn't have decimal numbers at the time
        // of writing, so I'll have to fudge the input.
        std::wistringstream val{stroke_width->get_value()};
        int whole;
        val >> whole;
        auto const left = val.rdbuf()->in_avail();
        int frac = 0;
        if (left >= 2)
        {
            // We have a . + 1 or 2 digits.
            val.ignore();    // Discard the .
            val >> frac;
            if (left == 2)
            {
                // We only had 1 digit, so multiply fractional part by 10
                frac *= 10;
            }
        }
        properties_[Indicator::Stroke_Width] = whole * 100 + frac;
    }

    /*
          <xs:element name="hover" minOccurs="0">
            <xs:complexType>
              <!-- FIXME ENSURE THIS HAS SOMETHING IN IT -->
    SCI_INDICSETHOVERSTYLE(int indicator, int indicatorStyle)
    SCI_INDICSETHOVERFORE(int indicator, colour fore)
    These messages set and get the colour and style used to draw indicators when
    the mouse is over them or the caret moved into them. The mouse cursor also
    changes when an indicator is drawn in hover style. The default is for the
    hover appearance to be the same as the normal appearance and calling
    SCI_INDICSETFORE or SCI_INDICSETSTYLE will also reset the hover attribute.
              <xs:all>
                <xs:element name="style" type="style" minOccurs="0"/>
                <xs:element name="colour" type="colour" minOccurs="0"/>
              </xs:all>
            </xs:complexType>
          </xs:element>
        </xs:all>
    */
}

}    // namespace Linter
