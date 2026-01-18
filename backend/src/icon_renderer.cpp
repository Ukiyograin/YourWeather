#include "icon_renderer.h"
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace std;

// 简单的SVG解析和渲染实现
class IconRenderer::IconRendererImpl {
public:
    IconRendererImpl() = default;
    ~IconRendererImpl() = default;
    
    vector<uint8_t> renderToBitmap(const vector<string>& paths, 
                                   int width, int height,
                                   const ColorTheme& theme) {
        vector<uint8_t> buffer(width * height * 4, 255); // RGBA格式，初始白色
        
        // 简化的渲染逻辑 - 实际项目中应该使用真正的SVG渲染库
        // 这里我们绘制简单的几何形状
        for (const auto& path : paths) {
            if (path.find("circle") != string::npos) {
                drawCircle(buffer, width, height, theme);
            } else if (path.find("rect") != string::npos) {
                drawRectangle(buffer, width, height, theme);
            } else if (path.find("polygon") != string::npos) {
                drawPolygon(buffer, width, height, theme);
            }
        }
        
        return buffer;
    }
    
private:
    void drawCircle(vector<uint8_t>& buffer, int width, int height, const ColorTheme& theme) {
        int centerX = width / 2;
        int centerY = height / 2;
        int radius = min(width, height) / 3;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int dx = x - centerX;
                int dy = y - centerY;
                if (dx * dx + dy * dy <= radius * radius) {
                    int index = (y * width + x) * 4;
                    buffer[index] = (theme.primary_color >> 16) & 0xFF;     // R
                    buffer[index + 1] = (theme.primary_color >> 8) & 0xFF;  // G
                    buffer[index + 2] = theme.primary_color & 0xFF;         // B
                    buffer[index + 3] = 255;                                // A
                }
            }
        }
    }
    
    void drawRectangle(vector<uint8_t>& buffer, int width, int height, const ColorTheme& theme) {
        int left = width / 4;
        int top = height / 4;
        int right = width * 3 / 4;
        int bottom = height * 3 / 4;
        
        for (int y = top; y < bottom; y++) {
            for (int x = left; x < right; x++) {
                int index = (y * width + x) * 4;
                buffer[index] = (theme.secondary_color >> 16) & 0xFF;
                buffer[index + 1] = (theme.secondary_color >> 8) & 0xFF;
                buffer[index + 2] = theme.secondary_color & 0xFF;
                buffer[index + 3] = 255;
            }
        }
    }
    
    void drawPolygon(vector<uint8_t>& buffer, int width, int height, const ColorTheme& theme) {
        // 绘制三角形
        vector<pair<int, int>> points = {
            {width / 2, height / 4},
            {width * 3 / 4, height * 3 / 4},
            {width / 4, height * 3 / 4}
        };
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (isPointInTriangle(x, y, points[0], points[1], points[2])) {
                    int index = (y * width + x) * 4;
                    buffer[index] = (theme.accent_color >> 16) & 0xFF;
                    buffer[index + 1] = (theme.accent_color >> 8) & 0xFF;
                    buffer[index + 2] = theme.accent_color & 0xFF;
                    buffer[index + 3] = 255;
                }
            }
        }
    }
    
    bool isPointInTriangle(int x, int y, 
                          const pair<int, int>& p1,
                          const pair<int, int>& p2,
                          const pair<int, int>& p3) {
        auto sign = [](const pair<int, int>& p1, const pair<int, int>& p2, const pair<int, int>& p3) {
            return (p1.first - p3.first) * (p2.second - p3.second) - 
                   (p2.first - p3.first) * (p1.second - p3.second);
        };
        
        int d1 = sign({x, y}, p1, p2);
        int d2 = sign({x, y}, p2, p3);
        int d3 = sign({x, y}, p3, p1);
        
        bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
        
        return !(has_neg && has_pos);
    }
};

// 图标定义
const vector<IconRenderer::IconDefinition> IconRenderer::icon_definitions_ = {
    {
        "sunny",
        {"M50,20 A30,30 0 1,1 50,80 A30,30 0 1,1 50,20 Z"},
        {{50, 20}, {80, 50}, {50, 80}, {20, 50}},
        true,
        true
    },
    {
        "cloudy",
        {
            "M20,50 Q35,30 50,30 Q65,30 80,50",
            "M20,50 C20,70 80,70 80,50"
        },
        {{20, 50}, {35, 30}, {65, 30}, {80, 50}},
        true,
        true
    },
    {
        "rain",
        {
            "M20,50 C20,70 80,70 80,50",
            "M35,70 L35,85",
            "M50,70 L50,85",
            "M65,70 L65,85"
        },
        {{20, 50}, {50, 30}, {80, 50}},
        true,
        true
    },
    {
        "snow",
        {
            "M20,50 C20,70 80,70 80,50",
            "M35,75 L45,85 M40,80 L40,90",
            "M50,75 L60,85 M55,80 L55,90",
            "M65,75 L75,85 M70,80 L70,90"
        },
        {{20, 50}, {50, 30}, {80, 50}},
        true,
        true
    },
    {
        "thunderstorm",
        {
            "M20,50 C20,70 80,70 80,50",
            "M40,65 L50,85 L45,80 L55,90"
        },
        {{20, 50}, {50, 30}, {80, 50}},
        true,
        true
    },
    {
        "fog",
        {
            "M20,45 L80,45",
            "M20,55 L80,55",
            "M20,65 L80,65"
        },
        {{20, 45}, {80, 45}},
        true,
        false
    },
    {
        "partly-cloudy-day",
        {
            "M30,30 A20,20 0 1,1 30,70 A20,20 0 1,1 30,30 Z",
            "M50,40 C50,60 90,60 90,40"
        },
        {{30, 30}, {30, 70}, {50, 40}, {90, 40}},
        true,
        true
    },
    {
        "partly-cloudy-night",
        {
            "M30,50 C30,35 45,25 60,30",
            "M50,40 C50,60 90,60 90,40"
        },
        {{30, 50}, {45, 25}, {60, 30}, {50, 40}},
        true,
        true
    },
    {
        "clear-night",
        {
            "M50,30 C30,40 40,70 50,70 C60,70 70,40 50,30 Z"
        },
        {{50, 30}, {30, 40}, {40, 70}, {60, 70}, {70, 40}},
        true,
        true
    },
    {
        "unknown",
        {
            "M50,20 A30,30 0 1,1 50,80 A30,30 0 1,1 50,20 Z",
            "M50,40 L50,55",
            "M50,60 L50,65"
        },
        {{50, 20}, {80, 50}, {50, 80}, {20, 50}},
        true,
        true
    }
};

// IconRenderer 公共方法实现
IconRenderer::IconRenderer() 
    : impl_(make_unique<IconRendererImpl>()) {
}

IconRenderer::~IconRenderer() = default;

vector<uint8_t> IconRenderer::renderIcon(const string& icon_name, 
                                        IconSize size,
                                        const ColorTheme& theme) {
    const IconDefinition* icon = findIconDefinition(icon_name);
    if (!icon) {
        icon = findIconDefinition("unknown");
        if (!icon) {
            throw runtime_error("找不到图标定义");
        }
    }
    
    int width = getIconWidth(size);
    int height = getIconHeight(size);
    
    return impl_->renderToBitmap(icon->svg_paths, width, height, theme);
}

bool IconRenderer::renderIconToFile(const string& icon_name, 
                                   const string& filename,
                                   IconSize size,
                                   const ColorTheme& theme) {
    try {
        auto buffer = renderIcon(icon_name, size, theme);
        
        // 简化的BMP文件写入
        return writeBMPFile(filename, buffer, getIconWidth(size), getIconHeight(size));
    } catch (const exception& e) {
        cerr << "渲染图标到文件失败: " << e.what() << endl;
        return false;
    }
}

string IconRenderer::getSVGPathData(const string& icon_name) {
    const IconDefinition* icon = findIconDefinition(icon_name);
    if (!icon || icon->svg_paths.empty()) {
        return "";
    }
    
    // 合并所有路径
    string result;
    for (const auto& path : icon->svg_paths) {
        if (!result.empty()) result += " ";
        result += path;
    }
    
    return result;
}

int IconRenderer::getIconWidth(IconSize size) {
    return static_cast<int>(size);
}

int IconRenderer::getIconHeight(IconSize size) {
    return static_cast<int>(size);
}

IconRenderer::ColorTheme IconRenderer::getDefaultTheme() {
    return {0x2196F3, 0x03A9F4, 0xFFFFFF, 0x00BCD4}; // Material Design 颜色
}

IconRenderer::ColorTheme IconRenderer::getDayTheme() {
    return {0xFF9800, 0xFFB74D, 0xFFFFFF, 0xFF9800}; // 橙色主题
}

IconRenderer::ColorTheme IconRenderer::getNightTheme() {
    return {0x3F51B5, 0x5C6BC0, 0x212121, 0x7986CB}; // 深蓝色主题
}

IconRenderer::ColorTheme IconRenderer::getRainTheme() {
    return {0x2196F3, 0x64B5F6, 0xFFFFFF, 0x1976D2}; // 蓝色主题
}

IconRenderer::ColorTheme IconRenderer::getSnowTheme() {
    return {0xE0E0E0, 0xF5F5F5, 0xFFFFFF, 0x9E9E9E}; // 灰色主题
}

const IconRenderer::IconDefinition* IconRenderer::findIconDefinition(const string& name) const {
    for (const auto& icon : icon_definitions_) {
        if (icon.name == name) {
            return &icon;
        }
    }
    return nullptr;
}

vector<uint8_t> IconRenderer::renderToBitmap(const IconDefinition& icon, 
                                            IconSize size,
                                            const ColorTheme& theme) {
    // 委托给实现类
    return impl_->renderToBitmap(icon.svg_paths, 
                                getIconWidth(size), 
                                getIconHeight(size), 
                                theme);
}

string IconRenderer::renderToSVG(const IconDefinition& icon,
                                const ColorTheme& theme) {
    stringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        << "width=\"100\" height=\"100\" viewBox=\"0 0 100 100\">\n";
    
    for (const auto& path : icon.svg_paths) {
        svg << "  <path d=\"" << path << "\" ";
        if (icon.has_fill) {
            svg << "fill=\"#" << hex << setw(6) << setfill('0') 
                << (theme.primary_color & 0xFFFFFF) << dec << "\" ";
        } else {
            svg << "fill=\"none\" ";
        }
        if (icon.has_stroke) {
            svg << "stroke=\"#" << hex << setw(6) << setfill('0')
                << (theme.secondary_color & 0xFFFFFF) << dec << "\" "
                << "stroke-width=\"2\" ";
        }
        svg << "/>\n";
    }
    
    svg << "</svg>";
    return svg.str();
}

bool IconRenderer::writeBMPFile(const string& filename, 
                               const vector<uint8_t>& buffer, 
                               int width, int height) {
#ifdef _WIN32
    HANDLE file = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // BMP文件头
    BITMAPFILEHEADER fileHeader = {0};
    BITMAPINFOHEADER infoHeader = {0};
    
    int rowSize = (width * 3 + 3) & ~3; // 每行字节数，4字节对齐
    int imageSize = rowSize * height;
    
    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageSize;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24; // RGB
    infoHeader.biCompression = BI_RGB;
    
    DWORD bytesWritten;
    WriteFile(file, &fileHeader, sizeof(fileHeader), &bytesWritten, NULL);
    WriteFile(file, &infoHeader, sizeof(infoHeader), &bytesWritten, NULL);
    
    // 写入像素数据（BGR格式）
    vector<uint8_t> bmpBuffer(imageSize, 0);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int srcIndex = ((height - 1 - y) * width + x) * 4; // BMP是倒着存储的
            int dstIndex = y * rowSize + x * 3;
            
            bmpBuffer[dstIndex] = buffer[srcIndex + 2];     // B
            bmpBuffer[dstIndex + 1] = buffer[srcIndex + 1]; // G
            bmpBuffer[dstIndex + 2] = buffer[srcIndex];     // R
        }
    }
    
    WriteFile(file, bmpBuffer.data(), imageSize, &bytesWritten, NULL);
    CloseHandle(file);
    
    return true;
#else
    // Linux/macOS 版本
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return false;
    }
 
    close(fd);
    return true;
#endif
}