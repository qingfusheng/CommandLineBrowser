#include<iostream>
#include <fstream>
#include <string>
#include <regex>
#include <vector>
#include <map>
#include <stack>
#include<cctype>
#include<unordered_map>
#include <sstream>
using namespace std;

// ��Ⱦ������С
const int SCREEN_WIDTH = 50;
const int SCREEN_HEIGHT = 10;
string screen[SCREEN_HEIGHT][SCREEN_WIDTH];
// ����������ʽ
unordered_map<string, string> attrs{
    {"red","\033[31m"},
    {"blue","\033[34m"},
    {"green","\033[32m"},
    {"em","\033[1m"},
    {"i","\033[3m"},
    {"u","\033[4m"},
    {"reset","\033[0m"},
};
// ö�����ͣ����뷽ʽ
enum class Alignment {
    Start,        // �����
    End,          // �Ҷ���
    Center,       // ���ж���
    SpaceEvenly   // �Ⱦ����
};

struct HTMLElement;
void initScreen();
void printScreen();
HTMLElement parseHTML(istream& file);
Alignment getAlignment(const string& value);
int cal_elem_width(const HTMLElement& element, bool isTotal);
int cal_elem_height(const HTMLElement& element, bool isTotal);
map<string, string> parseAttributes(const string& attrStr);
void inheritAttributes(HTMLElement& child, const map<string, string>& parentAttrs);
void renderHTMLToScreen(const HTMLElement& element, int x, int y, int w, int h);
void renderElementToScreen(const HTMLElement& element, int x, int y, int w, int h);
void renderContainerToScreen(const HTMLElement& element, int x, int y, int w, int h);


// ���ݽṹ����
struct HTMLElement {
    string tag;                            // ��ǩ����
    map<string, string> attrs;            // ����
    vector<HTMLElement> children;         // ��Ԫ��
    string text;                          // �ı�����
    /*void print(int indent = 0) const {
        string padding(indent, ' ');
        cout << padding << "Tag: " << tag << ", " << indent << "\n";
        cout << padding << "Attributes:\n";
        for (const auto& attr : attrs) {
            cout << padding << "  " << attr.first << " = " << attr.second << "\n";
        }
        cout << padding << "Text: _" << text << "_\n";
        cout << padding << "Children:\n";
        for (const auto& child : children) {
            child.print(indent + 2);
        }
        cout << "-------------------" << endl;
    }*/
};

// ���ߺ�������������
map<string, string> parseAttributes(const string& attrStr) {
    map<string, string> attrs;
    // ������ʽ��ƥ����ֵ����
    regex valueAttrRegex("(\\w[\\w-]*)=\"([^\"]*)\"");
    // ������ʽ��ƥ���ֵ���ԣ��������ԣ�
    regex boolAttrRegex(R"((\b(?:i|u|em)\b))");
    // ������ֵ����
    auto begin = sregex_iterator(attrStr.begin(), attrStr.end(), valueAttrRegex);
    auto end = sregex_iterator();
    for (auto &it = begin; it != end; ++it) {
        attrs[it->str(1)] = it->str(2);
    }

    // ������������
    begin = sregex_iterator(attrStr.begin(), attrStr.end(), boolAttrRegex);
    for (auto &it = begin; it != end; ++it) {
        attrs[it->str(1)] = "true";
    }
    return attrs;
}

// ���ߺ������̳����ԣ����º�İ汾��
void inheritAttributes(HTMLElement& child, const map<string, string>& parentAttrs) {
    // ����̳е������б�
    const vector<string> inheritableAttrs = { "color", "em", "i", "u" };

    // ����Ƿ�Ϊ�̳�Ŀ���ǩ
    if (child.tag != "div" && child.tag != "h" && child.tag != "p") {
        return;
    }

    for (const auto& [key, value] : parentAttrs) {
        // ���̳���������ԣ�������Ԫ��δ���������ʱ�ż̳�
        if (find(inheritableAttrs.begin(), inheritableAttrs.end(), key) != inheritableAttrs.end() &&
            child.attrs.find(key) == child.attrs.end()) {
            child.attrs[key] = value;
        }
    }
}

// ���ߺ���������HTML�����ɼ̳��߼���
HTMLElement parseHTML(istream& file) {
    HTMLElement root;
    stack<HTMLElement*> elementStack; // ջ�ṹά������������
    string buffer;
    char ch;
    bool inTag = false;  // ����һ����־λ�������Ƿ����ڴ����ǩ
    regex startTagRegex(R"(<(h|p|img|div)([^>]*)>)");
    regex endTagRegex(R"(</(h|p|img|div)>)");
    while (file.get(ch)) {
        if (ch == '<') {
            // �����ǩ�Ŀ�ʼ
            if (!buffer.empty()) {
                if (!elementStack.empty()) {
                    if (elementStack.top()->tag == "h" || elementStack.top()->tag == "p") {
                        elementStack.top()->text += buffer;  // ��֮ǰ���ı����ݼ��뵽��ǰԪ����
                    }
                }
                buffer.clear();
            }
            inTag = true;  // ���Ϊ�ڱ�ǩ�ڲ�
            buffer += ch;
        }
        else if (ch == '>') {
            // �����ǩ�Ľ���
            buffer += ch;
            smatch match;

            if (regex_search(buffer, match, startTagRegex)) {
                HTMLElement element;
                element.tag = match[1].str();  // ��ǩ��
                element.attrs = parseAttributes(match[2].str());  // ����

                if (elementStack.empty()) {
                    root = element;
                    elementStack.push(&root);
                }
                else {
                    // ���Լ̳У��Ӹ�Ԫ�ؼ̳����������
                    inheritAttributes(element, elementStack.top()->attrs);
                    // ����Ԫ����ӵ���ǰջ��Ԫ�ص���Ԫ����
                    elementStack.top()->children.push_back(element);
                    // ������Ԫ�ص�ָ��ѹ��ջ
                    elementStack.push(&elementStack.top()->children.back());
                }
            }
            else if (regex_search(buffer, match, endTagRegex)) {
                string endTag = match[1].str();
                if (!elementStack.empty() && elementStack.top()->tag == endTag) {
                    elementStack.pop();
                }
            }
            buffer.clear();
            inTag = false;  // ���Ϊ���ڱ�ǩ�ڲ�
        }
        else {
            if (inTag) {
                buffer += ch;  // ����������ǩ
            }
            else {
                buffer += ch;  // �ڱ�ǩ֮�⣬�����ı�����
            }
        }
    }
    return root;
}

// ��ȼ��㺯��
int cal_elem_width(const HTMLElement& element, bool isTotal=true) {
    /*
    isTotal: �Ƿ����Ϊdiv���ܿ�ȣ����Ϊtrue���򷵻�div���ܿ�ȣ�ֱ��ָ����value�������򷵻���Ԫ�سſ��Ŀ��
    */
    if (element.tag == "p" || element.tag == "h") {
        return static_cast<int>(element.text.size());
    }
    else if (element.tag == "img") {
        return stoi(element.attrs.at("width"));
    }
    else if (element.tag == "div") {
        if (isTotal && element.attrs.count("w")) {
            return stoi(element.attrs.at("w"));
        }
        else {
            string direction = element.attrs.count("direction") ? element.attrs.at("direction") : "row";
            if (direction == "row") {
                int max_width = 0;
                for (const auto& child : element.children) {
                    max_width = max(max_width, cal_elem_width(child, isTotal));
                }
                return max_width;
            }
            else { // column
                int total_width = 0;
                for (const auto& child : element.children) {
                    total_width += cal_elem_width(child, isTotal);
                }
                return total_width;
            }
        }
    }
    return 0;
}

// �߶ȼ��㺯��
int cal_elem_height(const HTMLElement& element, bool isTotal = true) {
    if (element.tag == "p" || element.tag == "h") {
        return 1;
    }
    else if (element.tag == "img") {
        int size = static_cast<int>(element.attrs.at("src").size());
        int width = stoi(element.attrs.at("width"));
        return (size + width - 1) / width; // ����ȡ��
    }
    else if (element.tag == "div") {
        if (isTotal && element.attrs.count("h")) {
            return stoi(element.attrs.at("h"));
        }
        else {
            string direction = element.attrs.count("direction") ? element.attrs.at("direction") : "row";
            if (direction == "row") {
                int total_height = 0;
                for (const auto& child : element.children) {
                    total_height += cal_elem_height(child, isTotal);
                }
                return total_height;
            }
            else { // column
                int max_height = 0;
                for (const auto& child : element.children) {
                    max_height = max(max_height, cal_elem_height(child, isTotal));
                }
                return max_height;
            }
        }
    }
    return 0;
}


// ��ʼ����Ⱦ����
void initScreen() {
    for (int i = 0; i < SCREEN_HEIGHT; ++i) {
        for (int j = 0; j < SCREEN_WIDTH; ++j) {
			screen[i][j] = "*";
		}
    }
}

// ��ӡ��Ⱦ����
void printScreen() {
    for (int i = 0; i < SCREEN_HEIGHT; ++i) {
        for (int j = 0; j < SCREEN_WIDTH; ++j) {
            printf("%s", screen[i][j].c_str());
        }
        cout << "\n";
    }
}

// ��Ⱦ����Ԫ�ص�����
void renderElementToScreen(const HTMLElement& element, int x, int y, int w, int h) {
    // cout << endl << "element_tag:" << element.tag << ", " << x << ", " << y << ", " << h << ", " << w << endl;
    if (element.tag == "p" || element.tag == "h") {
        const string& text = element.text;
        string stylePrefix = "";
        if (element.attrs.count("color")) {
            stylePrefix += attrs[element.attrs.at("color")];
		}
        if (element.attrs.count("em")) {
            stylePrefix += attrs["em"];
        }
        if (element.attrs.count("i")) {
            stylePrefix += attrs["i"];
		}
        if (element.attrs.count("u")) {
            stylePrefix += attrs["u"];
        }
        for (int j = 0; j < text.size(); ++j) {
            if (element.tag == "p") {
                screen[x][y + j] = text[j];
            }
            else {
                screen[x][y + j] = toupper(text[j]);
            }
        }
        if (stylePrefix != "") {
            screen[x][y] = stylePrefix + screen[x][y];
            screen[x][y + text.size() - 1] += attrs["reset"];
        }
    }
    else if (element.tag == "img") {
        if (!element.attrs.count("src") || !element.attrs.count("width")) {
            throw runtime_error("Image tag must have src and width attributes");
        }
        string content = element.attrs.at("src");
        // cout << "content:" << content.size() << endl;
        for (int i = 0; i < h && x + i < SCREEN_HEIGHT; ++i) {
            for (int j = 0; j < w && y + j < SCREEN_WIDTH; ++j) {
                screen[x + i][y + j] = content[i * w + j];
            }
        }
    }
}

static Alignment getAlignment(const std::string& value) {
    static const std::unordered_map<std::string, Alignment> alignmentMap = {
        {"start", Alignment::Start},
        {"end", Alignment::End},
        {"center", Alignment::Center},
        {"space-evenly", Alignment::SpaceEvenly}
    };
    auto it = alignmentMap.find(value);
    if (it != alignmentMap.end()) {
        return it->second;
    }
    throw std::runtime_error("Invalid alignment value");
}


// ��Ⱦ������Ԫ��
void renderContainerToScreen(const HTMLElement& element, int x, int y, int w, int h) {
    // cout << endl << "container_tag:" << element.tag << ", " << x << ", " << y << ", " << h << ", " << w << endl;
    string direction = element.attrs.count("direction") ? element.attrs.at("direction") : "row";
    // ��ȡ��������
    Alignment justifyContent = getAlignment(element.attrs.count("justify-content") ? element.attrs.at("justify-content") : "start");
    Alignment alignItems = getAlignment(element.attrs.count("align-items") ? element.attrs.at("align-items") : "start");

    int total_w = cal_elem_width(element, true);  // �������ܿ��
    int total_h = cal_elem_height(element, true);  // �������ܸ߶�
    int element_w = cal_elem_width(element, false);  //��Ԫ�ص��ܿ�ȣ���Ԫ�سſ��Ŀ�ȣ�
    int element_h = cal_elem_height(element, false);  // ��Ԫ�ص��ܸ߶ȣ���Ԫ�سſ��ĸ߶ȣ�
    // cout << "element_w_h:" << element_h << ", " << element_w << endl;
    //cout<<"justify-content:"<< (element.attrs.count("justify-content") ? element.attrs.at("justify-content") : "start") <<", align-items:"<< (element.attrs.count("align-items") ? element.attrs.at("align-items") : "start") <<endl;
    int init_h = 0, gap_h = 0, init_w = 0, gap_w = 0;
    
    //cout << "init_h:" << init_h << ", gap_h:" << gap_h << ", init_w:" << init_w << ", gap_w:" << gap_w << endl;
    if (direction == "row") {
        switch (alignItems) {
        case Alignment::Start:
            init_h = 0;
            gap_h = 0;
            break;
        case Alignment::Center:
            init_h = (total_h - element_h) / 2;
            gap_h = 0;
            break;
        case Alignment::End:
            init_h = total_h - element_h;
            gap_h = 0;
            break;
        case Alignment::SpaceEvenly:
            init_h = static_cast<int>((total_h - element_h) / static_cast<int>(element.children.size() + 1));
            gap_h = init_h;
            break;
        default:
            throw runtime_error("Invalid justify-content value");
        }
        int childX = x + init_h;
        for (const auto& child : element.children) {
            int childW = cal_elem_width(child, true);
            int childH = cal_elem_height(child, true);
            switch (justifyContent) {
            case Alignment::Start:
                init_w = 0;
                gap_w = 0;
                break;
            case Alignment::End:
                init_w = total_w - childW;
                gap_w = 0;
                break;
            case Alignment::Center:
                init_w = (total_w - childW) / 2;
                gap_w = 0;
                break;
            case Alignment::SpaceEvenly:
                init_w = static_cast<int>((total_w - childW) / static_cast<int>(element.children.size() + 1));
                gap_w = init_w;
                break;
            }
            //cout << "child_element:" << child.tag << ",child_w_h:" << childH << ", " << childW << endl;
            // ��Ⱦ��Ԫ��
            renderHTMLToScreen(child, childX, y + init_w, childW, childH);
            // ����ˮƽλ��
            childX += childH + gap_h;

        }
    }
    else { // column
        switch (justifyContent) {
        case Alignment::Start:
            init_w = 0;
            gap_w = 0;
            break;
        case Alignment::End:
            init_w = total_w - element_w;
            gap_w = 0;
            break;
        case Alignment::Center:
            init_w = (total_w - element_w) / 2;
            gap_w = 0;
            break;
        case Alignment::SpaceEvenly:
            init_w = static_cast<int>((total_w - element_w) / static_cast<int>(element.children.size() + 1));
            gap_w = init_w;
            break;
        }

        int childY = y + init_w; // ��ֱ������ʼλ��
        for (const auto& child : element.children) {
            int childW = cal_elem_width(child, true);
            int childH = cal_elem_height(child, true);
            switch (alignItems) {
            case Alignment::Start:
                init_h = 0;
                gap_h = 0;
                break;
            case Alignment::Center:
                init_h = (total_h - childH) / 2;
                break;
            case Alignment::End:
                init_h = total_h - childH;
                gap_h = 0;
                break;
            case Alignment::SpaceEvenly:
                init_h = static_cast<int>((total_h - childH) / static_cast<int>(element.children.size() + 1));
                gap_h = init_h;
                break;
            default:
                throw runtime_error("Invalid justify-content value");
            }
            // ��Ⱦ��Ԫ��
            renderHTMLToScreen(child, x + init_h, childY, childW, childH);
            // ���´�ֱλ��
            childY += childW + gap_w;
        }
    }
}

// ��ȾHTML������
void renderHTMLToScreen(const HTMLElement& element, int x = 0, int y = 0, int w = SCREEN_WIDTH, int h = SCREEN_HEIGHT) {
    if (element.tag == "div") {
        renderContainerToScreen(element, x, y, w, h);
    }
    else {
        renderElementToScreen(element, x, y, w, h);
    }
}

// ������
int main() {
    const string filename = "input.txt";

    // ���ļ�
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Unable to open file " << filename << endl;
        return 1;
    }
    //// ֱ�ӽ����ļ�
    //HTMLElement root = parseHTML(file);
    //file.close();
    // ��ȡ�ļ����ݵ��ַ��������ǰ��׺
    std::string augmentedContent = "<div>\n" + std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()) + "\n</div>";
    file.close();

    // ���ַ���ת��Ϊ�������ݸ� parseHTML
    istringstream augmentedStream(augmentedContent);
    HTMLElement root = parseHTML(augmentedStream);

    //// ����������
    //root.print();
    //cout << endl << endl;
    initScreen();
    renderHTMLToScreen(root);
    printScreen();
    return 0;
}