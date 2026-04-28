#include "pch.h"
#include "MdConverter.h"
#include <sstream>

// ── helpers ───────────────────────────────────────────────────

static std::wstring HtmlEsc(const std::wstring& s)
{
    std::wstring out;
    out.reserve(s.size() + 8);
    for (wchar_t c : s)
    {
        switch (c)
        {
        case L'&':  out += L"&amp;";  break;
        case L'<':  out += L"&lt;";   break;
        case L'>':  out += L"&gt;";   break;
        case L'"':  out += L"&quot;"; break;
        default:    out += c;          break;
        }
    }
    return out;
}

static std::wstring Trim(const std::wstring& s)
{
    auto l = s.find_first_not_of(L" \t\r\n");
    if (l == std::wstring::npos) return L"";
    auto r = s.find_last_not_of(L" \t\r\n");
    return s.substr(l, r - l + 1);
}

static std::vector<std::wstring> SplitLines(const std::wstring& text)
{
    std::vector<std::wstring> lines;
    std::wistringstream ss(text);
    std::wstring ln;
    while (std::getline(ss, ln))
    {
        if (!ln.empty() && ln.back() == L'\r') ln.pop_back();
        lines.push_back(ln);
    }
    return lines;
}

static bool StartsOrderedItem(const std::wstring& line, size_t& dotPos)
{
    dotPos = line.find(L'.');
    if (dotPos == std::wstring::npos || dotPos == 0 || dotPos + 1 >= line.size() || line[dotPos + 1] != L' ')
        return false;
    for (size_t k = 0; k < dotPos; ++k)
        if (!std::isdigit((unsigned char)line[k])) return false;
    return true;
}

static bool StartsUnorderedItem(const std::wstring& line)
{
    return line.size() >= 2 && (line[0] == L'-' || line[0] == L'*' || line[0] == L'+') && line[1] == L' ';
}

static bool IsHr(const std::wstring& line)
{
    std::wstring t = Trim(line);
    return t == L"---" || t == L"***" || t == L"___"
        || t == L"- - -" || t == L"* * *" || t == L"_ _ _";
}

// ── inline parser ─────────────────────────────────────────────
// Processes: ***bold italic***, **bold**, *italic*, `code`, ![img](url), [link](url)
static std::wstring Inline(const std::wstring& text)
{
    std::wstring out;
    size_t i = 0;
    const size_t n = text.size();

    while (i < n)
    {
        // Code span
        if (text[i] == L'`')
        {
            size_t j = text.find(L'`', i + 1);
            if (j != std::wstring::npos)
            {
                out += L"<code>" + HtmlEsc(text.substr(i + 1, j - i - 1)) + L"</code>";
                i = j + 1; continue;
            }
        }

        // Bold+italic ***
        if (i + 2 < n && text[i] == L'*' && text[i+1] == L'*' && text[i+2] == L'*')
        {
            size_t j = text.find(L"***", i + 3);
            if (j != std::wstring::npos)
            {
                out += L"<strong><em>" + Inline(text.substr(i + 3, j - i - 3)) + L"</em></strong>";
                i = j + 3; continue;
            }
        }

        // Bold **
        if (i + 1 < n && text[i] == L'*' && text[i+1] == L'*')
        {
            size_t j = text.find(L"**", i + 2);
            if (j != std::wstring::npos)
            {
                out += L"<strong>" + Inline(text.substr(i + 2, j - i - 2)) + L"</strong>";
                i = j + 2; continue;
            }
        }

        // Italic *
        if (text[i] == L'*')
        {
            size_t j = text.find(L'*', i + 1);
            if (j != std::wstring::npos && (j == i + 1 || text[j-1] != L'*'))
            {
                out += L"<em>" + Inline(text.substr(i + 1, j - i - 1)) + L"</em>";
                i = j + 1; continue;
            }
        }

        // Image ![alt](url)
        if (i + 1 < n && text[i] == L'!' && text[i+1] == L'[')
        {
            size_t j = text.find(L']', i + 2);
            if (j != std::wstring::npos && j + 1 < n && text[j+1] == L'(')
            {
                size_t k = text.find(L')', j + 2);
                if (k != std::wstring::npos)
                {
                    out += L"<img src=\"" + HtmlEsc(text.substr(j + 2, k - j - 2))
                         + L"\" alt=\"" + HtmlEsc(text.substr(i + 2, j - i - 2)) + L"\">";
                    i = k + 1; continue;
                }
            }
        }

        // Link [text](url)
        if (text[i] == L'[')
        {
            size_t j = text.find(L']', i + 1);
            if (j != std::wstring::npos && j + 1 < n && text[j+1] == L'(')
            {
                size_t k = text.find(L')', j + 2);
                if (k != std::wstring::npos)
                {
                    out += L"<a href=\"" + HtmlEsc(text.substr(j + 2, k - j - 2))
                         + L"\">" + Inline(text.substr(i + 1, j - i - 1)) + L"</a>";
                    i = k + 1; continue;
                }
            }
        }

        // HTML escape
        switch (text[i])
        {
        case L'&': out += L"&amp;";  break;
        case L'<': out += L"&lt;";   break;
        case L'>': out += L"&gt;";   break;
        case L'"': out += L"&quot;"; break;
        default:   out += text[i];   break;
        }
        ++i;
    }
    return out;
}

// ── RTF helpers ───────────────────────────────────────────────

// Append text to RTF, escaping RTF metacharacters and encoding
// non-ASCII as \uN? (signed 16-bit Unicode code point).
static void RtfText(std::string& out, const std::wstring& s)
{
    for (wchar_t c : s)
    {
        if (c < 128)
        {
            switch (c)
            {
            case '\\': out += "\\\\"; break;
            case '{':  out += "\\{";  break;
            case '}':  out += "\\}";  break;
            default:   out += (char)c; break;
            }
        }
        else
        {
            int n = (c > 32767) ? ((int)c - 65536) : (int)c;
            out += "\\u" + std::to_string(n) + "?";
        }
    }
}

static void InlineRtf(std::string& out, const std::wstring& text);

static void InlineRtf(std::string& out, const std::wstring& text)
{
    size_t i = 0;
    const size_t n = text.size();

    while (i < n)
    {
        if (text[i] == L'`')
        {
            size_t j = text.find(L'`', i + 1);
            if (j != std::wstring::npos)
            {
                out += "{\\f1\\cf4 "; RtfText(out, text.substr(i+1, j-i-1)); out += "}";
                i = j + 1; continue;
            }
        }
        if (i+2 < n && text[i]==L'*' && text[i+1]==L'*' && text[i+2]==L'*')
        {
            size_t j = text.find(L"***", i + 3);
            if (j != std::wstring::npos)
            {
                out += "{\\b\\i "; InlineRtf(out, text.substr(i+3, j-i-3)); out += "}";
                i = j + 3; continue;
            }
        }
        if (i+1 < n && text[i]==L'*' && text[i+1]==L'*')
        {
            size_t j = text.find(L"**", i + 2);
            if (j != std::wstring::npos)
            {
                out += "{\\b "; InlineRtf(out, text.substr(i+2, j-i-2)); out += "}";
                i = j + 2; continue;
            }
        }
        if (text[i] == L'*')
        {
            size_t j = text.find(L'*', i + 1);
            if (j != std::wstring::npos)
            {
                out += "{\\i "; InlineRtf(out, text.substr(i+1, j-i-1)); out += "}";
                i = j + 1; continue;
            }
        }
        if (text[i] == L'[')
        {
            size_t j = text.find(L']', i + 1);
            if (j != std::wstring::npos && j+1 < n && text[j+1] == L'(')
            {
                size_t k = text.find(L')', j + 2);
                if (k != std::wstring::npos)
                {
                    out += "{\\cf3\\ul "; InlineRtf(out, text.substr(i+1, j-i-1)); out += "}";
                    i = k + 1; continue;
                }
            }
        }
        RtfText(out, text.substr(i, 1));
        ++i;
    }
}

// ── ToRtf ─────────────────────────────────────────────────────

std::string MdConverter::ToRtf(const CString& mdText)
{
    std::wstring md(mdText.GetString());
    auto lines = SplitLines(md);
    const size_t N = lines.size();

    std::string out;
    out.reserve(md.size() * 3);

    // RTF header: two fonts, four colours
    out += "{\\rtf1\\ansi\\deff0"
           "{\\fonttbl{\\f0\\fswiss\\fcharset0 Segoe UI;}{\\f1\\fmodern\\fcharset0 Consolas;}}"
           "{\\colortbl;"
             "\\red36\\green41\\blue46;"   // cf1 body text
             "\\red87\\green96\\blue106;"  // cf2 muted
             "\\red3\\green102\\blue214;"  // cf3 link
             "\\red108\\green117\\blue125;}" // cf4 code
           "\\widowctrl\\f0\\fs22\\cf1\\lang1042 ";

    size_t i = 0;
    while (i < N)
    {
        const std::wstring& line = lines[i];
        if (Trim(line).empty()) { ++i; continue; }

        // Heading
        if (!line.empty() && line[0] == L'#')
        {
            int lv = 0;
            while (lv < (int)line.size() && line[lv] == L'#') ++lv;
            if (lv <= 6 && (lv == (int)line.size() || line[lv] == L' '))
            {
                static const char* sz[] = {"","\\fs48","\\fs38","\\fs28","\\fs24","\\fs22","\\fs22"};
                std::wstring content = lv < (int)line.size() ? Trim(line.substr(lv)) : L"";
                out += "\\pard\\sb200\\sa80\\sl276\\slmult1{\\b"; out += sz[lv]; out += " ";
                InlineRtf(out, content);
                out += "}\\par\\pard\n";
                ++i; continue;
            }
        }

        // Fenced code block
        if (line.size() >= 3 && line.substr(0,3) == L"```")
        {
            out += "\\pard\\sb60\\sa60\\sl240\\slmult1{\\f1\\fs18\\cf4 ";
            ++i;
            while (i < N && !(lines[i].size() >= 3 && lines[i].substr(0,3) == L"```"))
            {
                RtfText(out, lines[i]); out += "\\line\n"; ++i;
            }
            if (i < N) ++i;
            out += "}\\par\\pard\n"; continue;
        }

        // Blockquote
        if (!line.empty() && line[0] == L'>')
        {
            out += "\\pard\\li720\\ri720\\sb60\\sa60\\sl276\\slmult1{\\cf2\\i ";
            while (i < N && !lines[i].empty() && lines[i][0] == L'>')
            {
                std::wstring ql = lines[i].substr(1);
                if (!ql.empty() && ql[0] == L' ') ql = ql.substr(1);
                InlineRtf(out, Trim(ql)); out += " "; ++i;
            }
            out += "}\\par\\pard\n"; continue;
        }

        // HR
        if (IsHr(line))
        {
            out += "\\pard\\brdrb\\brdrs\\brdrw10\\brsp100\\par\\pard\n";
            ++i; continue;
        }

        // Unordered list
        if (StartsUnorderedItem(line))
        {
            while (i < N && StartsUnorderedItem(lines[i]))
            {
                out += "\\pard\\fi-360\\li720\\sl276\\slmult1\\bullet  ";
                InlineRtf(out, Trim(lines[i].substr(2)));
                out += "\\par\\pard\n"; ++i;
            }
            continue;
        }

        // Ordered list
        {
            size_t dotPos = 0;
            if (StartsOrderedItem(line, dotPos))
            {
                int num = 1;
                while (i < N && StartsOrderedItem(lines[i], dotPos))
                {
                    out += "\\pard\\fi-360\\li720\\sl276\\slmult1 ";
                    out += std::to_string(num++) + ". ";
                    InlineRtf(out, Trim(lines[i].substr(dotPos + 2)));
                    out += "\\par\\pard\n"; ++i;
                }
                continue;
            }
        }

        // Paragraph
        std::wstring para;
        while (i < N)
        {
            const std::wstring& ln = lines[i];
            if (Trim(ln).empty()) break;
            if (!ln.empty() && ln[0] == L'#') break;
            if (ln.size() >= 3 && ln.substr(0,3) == L"```") break;
            if (!ln.empty() && ln[0] == L'>') break;
            if (IsHr(ln)) break;
            if (StartsUnorderedItem(ln)) break;
            size_t dp = 0; if (StartsOrderedItem(ln, dp)) break;
            if (!para.empty()) para += L" ";
            para += Trim(ln); ++i;
        }
        if (!para.empty())
        {
            out += "\\pard\\sl276\\slmult1\\sb40\\sa40 ";
            InlineRtf(out, para);
            out += "\\par\\pard\n";
        }
    }

    out += "}";
    return out;
}

// ── CSS template ─────────────────────────────────────────────

static const wchar_t* kCss =
    L"body{font-family:-apple-system,'Segoe UI',Helvetica,Arial,sans-serif;"
    L"max-width:800px;margin:40px auto;padding:0 24px;line-height:1.7;color:#24292f}"
    L"h1,h2,h3,h4,h5,h6{margin-top:1.4em;margin-bottom:.5em;font-weight:600}"
    L"h1{font-size:2em;border-bottom:2px solid #d0d7de;padding-bottom:.3em}"
    L"h2{font-size:1.5em;border-bottom:1px solid #d0d7de;padding-bottom:.2em}"
    L"h3{font-size:1.25em}h4{font-size:1em}h5{font-size:.875em}h6{font-size:.85em;color:#57606a}"
    L"p{margin:.6em 0}"
    L"code{background:#f6f8fa;padding:.2em .4em;border-radius:4px;"
    L"font-family:'Consolas','Courier New',monospace;font-size:.88em}"
    L"pre{background:#f6f8fa;padding:16px;border-radius:6px;overflow-x:auto;margin:1em 0}"
    L"pre code{background:none;padding:0;font-size:.88em}"
    L"blockquote{border-left:4px solid #d0d7de;margin:.8em 0;padding:.4em 1em;color:#57606a}"
    L"blockquote p{margin:0}"
    L"hr{border:none;border-top:1px solid #d0d7de;margin:2em 0}"
    L"a{color:#0969da;text-decoration:none}a:hover{text-decoration:underline}"
    L"img{max-width:100%;height:auto}"
    L"ul,ol{padding-left:2em;margin:.5em 0}li{margin:.25em 0}"
    L"table{border-collapse:collapse;width:100%;margin:1em 0}"
    L"th,td{border:1px solid #d0d7de;padding:6px 12px}th{background:#f6f8fa}";

// ── block-level parser ────────────────────────────────────────

CString MdConverter::ToHtml(const CString& mdText, const CString& title)
{
    std::wstring md(mdText.GetString());
    auto lines = SplitLines(md);
    const size_t N = lines.size();
    std::wstring body;
    size_t i = 0;

    while (i < N)
    {
        const std::wstring& line = lines[i];

        // blank line
        if (Trim(line).empty()) { ++i; continue; }

        // heading
        if (!line.empty() && line[0] == L'#')
        {
            int lv = 0;
            while (lv < (int)line.size() && line[lv] == L'#') ++lv;
            if (lv <= 6 && (lv == (int)line.size() || line[lv] == L' '))
            {
                std::wstring content = lv < (int)line.size() ? Trim(line.substr(lv)) : L"";
                std::wstring lvs = std::to_wstring(lv);
                body += L"<h" + lvs + L">" + Inline(content) + L"</h" + lvs + L">\n";
                ++i; continue;
            }
        }

        // fenced code block
        if (line.size() >= 3 && line.substr(0, 3) == L"```")
        {
            std::wstring lang = Trim(line.substr(3));
            std::wstring code;
            ++i;
            while (i < N && !(lines[i].size() >= 3 && lines[i].substr(0, 3) == L"```"))
                code += HtmlEsc(lines[i++]) + L"\n";
            if (i < N) ++i;
            std::wstring cls = lang.empty() ? L"" : L" class=\"language-" + HtmlEsc(lang) + L"\"";
            body += L"<pre><code" + cls + L">" + code + L"</code></pre>\n";
            continue;
        }

        // blockquote
        if (!line.empty() && line[0] == L'>')
        {
            std::wstring qcontent;
            while (i < N && !lines[i].empty() && lines[i][0] == L'>')
            {
                std::wstring ql = lines[i].substr(1);
                if (!ql.empty() && ql[0] == L' ') ql = ql.substr(1);
                qcontent += Inline(Trim(ql)) + L" ";
                ++i;
            }
            body += L"<blockquote><p>" + Trim(qcontent) + L"</p></blockquote>\n";
            continue;
        }

        // horizontal rule
        if (IsHr(line)) { body += L"<hr>\n"; ++i; continue; }

        // unordered list
        if (StartsUnorderedItem(line))
        {
            body += L"<ul>\n";
            while (i < N && StartsUnorderedItem(lines[i]))
            {
                body += L"<li>" + Inline(Trim(lines[i].substr(2))) + L"</li>\n";
                ++i;
            }
            body += L"</ul>\n";
            continue;
        }

        // ordered list
        {
            size_t dotPos = 0;
            if (StartsOrderedItem(line, dotPos))
            {
                body += L"<ol>\n";
                while (i < N && StartsOrderedItem(lines[i], dotPos))
                {
                    body += L"<li>" + Inline(Trim(lines[i].substr(dotPos + 2))) + L"</li>\n";
                    ++i;
                }
                body += L"</ol>\n";
                continue;
            }
        }

        // paragraph — collect consecutive non-block lines
        std::wstring para;
        while (i < N)
        {
            const std::wstring& ln = lines[i];
            if (Trim(ln).empty()) break;
            if (!ln.empty() && ln[0] == L'#') break;
            if (ln.size() >= 3 && ln.substr(0, 3) == L"```") break;
            if (!ln.empty() && ln[0] == L'>') break;
            if (IsHr(ln)) break;
            if (StartsUnorderedItem(ln)) break;
            size_t dp = 0;
            if (StartsOrderedItem(ln, dp)) break;
            if (!para.empty()) para += L" ";
            para += Trim(ln);
            ++i;
        }
        if (!para.empty())
            body += L"<p>" + Inline(para) + L"</p>\n";
    }

    std::wstring t = title.IsEmpty() ? L"Document" : std::wstring(title.GetString());
    std::wstring html =
        L"<!DOCTYPE html>\n<html lang=\"ko\">\n<head>\n"
        L"<meta charset=\"UTF-8\">\n"
        L"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
        L"<title>" + HtmlEsc(t) + L"</title>\n"
        L"<style>" + std::wstring(kCss) + L"</style>\n"
        L"</head>\n<body>\n" + body + L"</body>\n</html>\n";

    return CString(html.c_str());
}
