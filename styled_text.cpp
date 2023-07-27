#include "styled_text.h"
#include "formatted.h"
#include <boost/algorithm/string/join.hpp>

/************************************************************************
 * Static data
 ***********************************************************************/

vector<pair<styled_text::color_e, U16>> styled_text::color_data {
    { styled_text::color_none, 0 },
    { styled_text::black, 232 },
    { styled_text::red, 9 },
    { styled_text::green, 40 },
    { styled_text::yellow, 11 },
    { styled_text::blue, 20 },
    { styled_text::magenta, 90 },
    { styled_text::cyan, 14 },
    { styled_text::white, 15 },
    { styled_text::grey, 244 },
    { styled_text::deep_blue, 20 },
    { styled_text::mid_blue, 27 },
    { styled_text::orange, 208 },
    { styled_text::pink, 201 },
    { styled_text::brown, 1 },
};

vector<pair<styled_text::style_e, U16>> styled_text::style_data {
    { styled_text::style_none, 0 },
    { styled_text::bold, 1 },
    { styled_text::italic, 3 },
    { styled_text::blink, 5 },
    { styled_text::underline, 4 },
    { styled_text::crossed, 9 },
    { styled_text::inverted, 7 },
};

vector<U16> styled_text::color_map;
vector<U16> styled_text::style_map;

string styled_text::escape_ch = "\u001b";

styled_text::render_e styled_text::renderer = plain;

/************************************************************************
 * Constructors
 ***********************************************************************/

styled_text::styled_text(const string &t, color_e c, color_e bg, style_e s, size_t sz)
    : text(t), color(c), background(bg), style(s), size(sz)
{
}

styled_text::styled_text(const styled_text &st)
    : text(st.text), color(st.color), background(st.background), style(st.style), size(st.size)
{
    for (const styled_text &t : st.subtexts) {
        subtexts.emplace_back(t);
    }
}

styled_text &styled_text::append(const string &t)
{
    if (subtexts.empty()) {
        text += t;
    } else {
        subtexts.back().text += t;
    }
    return *this;
}

styled_text &styled_text::append(const styled_text &st)
{
    if (subtexts.empty() && !text.empty()) {
        subtexts.emplace_back(*this);
        text.clear();
    }
    subtexts.emplace_back(st);
    return *this;
}


styled_text &styled_text::override(color_e c, color_e bg, style_e s, size_t sz)
{
    set_color(c);
    set_background(bg);
    set_style(s);
    set_size(sz);
    return *this;
}

styled_text &styled_text::override(const styled_text &st)
{
    set_color(st.color);
    set_background(st.background);
    set_style(st.style);
    set_size(st.size);
    return *this;
}

styled_text &styled_text::underride(color_e c, color_e bg, style_e s, size_t sz)
{
    underride_color(c);
    underride_background(bg);
    underride_style(s);
    underride_size(sz);
    return *this;
}


styled_text &styled_text::underride(const styled_text &st)
{
    underride_color(st.color);
    underride_background(st.background);
    underride_style(st.style);
    underride_size(st.size);
    return *this;
}

string styled_text::str() const
{
    if (subtexts.empty()) {
        return text;
    } else {
        vector<string> strs;
        for (const styled_text &st : subtexts) {
            strs.emplace_back(st.str());
        }
        return boost::algorithm::join(strs, "");
    }
}

string styled_text::render(render_e r) const
{
    switch (r) {
    case plain:
        return render_plain();
    case iso6429:
        return render_iso6429();
    default:
        return render_plain();
    }
}

string styled_text::render_plain() const
{
    return str();
}

string styled_text::render_iso6429() const
{
    make_maps();
    string result;
    if (subtexts.empty()) {
        result = render_style_iso6429(style) +
            render_color_iso6429(color, false) +
            render_color_iso6429(background, true) +
            text;
    } else {
        for (const styled_text &st : subtexts) {
            styled_text st1(st);
            st1.underride(*this);
            result += st1.render_iso6429();
        }
    }
    result += render_style_iso6429(style_none);
    return result;
}

string styled_text::render_color_iso6429(color_e c, bool bg)
{
    U16 op = bg ? op_bg : op_fg;
    string result;
    if (c==color_none) {
        result = formatted("\e[%dm", op + 1); 
    } else {
        result = formatted("\e[%d:5:%dm", op, color_map[c]);
    }
    return result;
}

string styled_text::render_style_iso6429(style_e s)
{
    string result;
    if (s == style_none) {
        result = "\e[0m";
    } else {
        U16 ss = s;
        while (ss != 0) {
            U16 ss_new = ss & (ss - 1);
            U16 this_s = ss ^ ss_new;
            ss = ss_new;
            result += formatted("\u001b[%dm", style_map[this_s]);
        }
    }
    return result;
}

void styled_text::make_maps()
{
    if (color_map.empty()) {
        color_map.resize(color_final);
        for (const auto &cd : color_data) {
            color_map[cd.first] = cd.second;
        }
        style_map.resize(style_final);
        for (const auto &sd : style_data) {
            style_map[sd.first] = sd.second;
        }
    }
}
