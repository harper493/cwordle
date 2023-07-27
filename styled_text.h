#ifndef __STYLED_TEXT
#define __STYLED_TEXT

#include "types.h"

/************************************************************************
 * style_text - represent text including color and styling
 ***********************************************************************/

class styled_text
{
public:
    enum color_e {
        color_none,
        black,
        red,
        green,
        yellow,
        blue,
        magenta,
        cyan,
        white,
        grey,
        deep_blue,
        mid_blue,
        orange,
        pink,
        brown,
        color_final,
    };
    enum style_e {
        style_none = 0,
        bold = 0x01,
        italic = 0x02,
        blink = 0x04,
        underline = 0x08,
        crossed = 0x10,
        inverted = 0x20,
        style_final = inverted,
    };
    enum render_e {
        plain,
        iso6429,
    };
    enum chars_e {
        ch_escape = 0x1b,
    };
    enum ops_e {
        op_fg = 38,
        op_bg = 48,
    };
private:
    string text = "";
    color_e color = color_none;
    color_e background = color_none;
    style_e style = style_none;
    U16 size = 0;
    vector<styled_text> subtexts;
    static render_e renderer;
    static vector<pair<color_e, U16>> color_data;
    static vector<pair<style_e, U16>> style_data;
    static vector<U16> color_map;
    static vector<U16> style_map;
    static string escape_ch;
public:
    styled_text() { };
    styled_text(const string &t, color_e c=color_none, color_e bg=color_none, style_e s=style_none, size_t sz=0);
    styled_text(const styled_text &st);
    template<class ITER>
    styled_text(ITER begin, ITER end)
    {
        while (begin != end) {
            append(*begin);
        }
    }
    template<class COLL, typename boost::enable_if<boost::is_same<typename COLL::value_type, styled_text>>::type>
    styled_text(COLL coll)
    {
        for (const styled_text &st : coll) {
            append(st);
        }
    }
    bool empty() const { return text.empty() && subtexts.empty(); };
    styled_text &append(const string &t);
    styled_text &operator+=(const string &t)
    {
        return append(t);
    }
    styled_text &append(const styled_text &st);
    styled_text &append(const string &t, color_e c, color_e bg=color_none, style_e s=style_none, size_t sz=0)
    {
        return append(styled_text(t, c, bg, s, sz));
    }
    styled_text &operator+=(const styled_text &st)
    {
        return append(st);
    }
    styled_text &override(color_e c=color_none, color_e bg=color_none, style_e s=style_none, size_t sz=0);
    styled_text &override(const styled_text &st);
    styled_text &underride(color_e c=color_none, color_e bg=color_none, style_e s=style_none, size_t sz=0);
    styled_text &underride(const styled_text &st);
    styled_text &set_color(color_e c)
    {
        if (c != color_none) {
            color = c;
        }
        return *this;
    }
    styled_text &set_background(color_e c)
    {
        if (c != color_none) {
            background = c;
        }
        return *this;
    }
    styled_text &set_style(style_e s)
    {
        if (s != style_none) {
            style = s;
        }
        return *this;
    }
    styled_text &set_size(U16 sz)
    {
        if (sz > 0) {
            size = sz;
        }
        return *this;    }
    styled_text &underride_color(color_e c)
    {
        if (color == color_none) {
            color = c;
        }
        return *this;
    }
    styled_text &underride_background(color_e c)
    {
        if (background != color_none) {
            background = c;
        }
        return *this;
    }
    styled_text &underride_style(style_e s)
    {
        if (style == style_none) {
            style = s;
        }
        return *this;
    }
    styled_text &underride_size(U16 sz)
    {
        if (size == 0) {
            size = sz;
        }
        return *this;
    }
    string str() const;
    string render() const
    {
        return render(renderer);
    }
    string render(render_e r) const;
    string render_plain() const;
    string render_iso6429() const;
    static void set_renderer(render_e r)
    {
        renderer = r;
    }
private:
    static string render_color_iso6429(color_e c, bool bg);
    static string render_style_iso6429(style_e s);
    static void  make_maps();
};

inline std::ostream &operator<<(std::ostream &ostr, const styled_text &st)
{
    ostr << st.render();
    return ostr;
}

#endif
