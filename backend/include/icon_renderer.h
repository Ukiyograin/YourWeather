#ifndef ICON_RENDERER_H
#define ICON_RENDERER_H

#include <string>
#include <vector>
#include <cstdint>

// 矢量图标渲染器
class IconRenderer {
public:
    // 图标大小
    enum IconSize {
        SMALL = 16,
        MEDIUM = 32,
        LARGE = 64,
        XLARGE = 128
    };
    
    // 颜色主题
    struct ColorTheme {
        uint32_t primary_color;
        uint32_t secondary_color;
        uint32_t background_color;
        uint32_t accent_color;
    };
    
    IconRenderer();
    ~IconRenderer();
    
    // 渲染图标到内存
    std::vector<uint8_t> renderIcon(const std::string& icon_name, 
                                    IconSize size = MEDIUM,
                                    const ColorTheme& theme = getDefaultTheme());
    
    // 渲染图标到文件
    bool renderIconToFile(const std::string& icon_name, 
                         const std::string& filename,
                         IconSize size = MEDIUM,
                         const ColorTheme& theme = getDefaultTheme());
    
    // 获取SVG路径数据（用于WPF）
    std::string getSVGPathData(const std::string& icon_name);
    
    // 获取图标尺寸信息
    static int getIconWidth(IconSize size);
    static int getIconHeight(IconSize size);
    
    // 默认颜色主题
    static ColorTheme getDefaultTheme();
    static ColorTheme getDayTheme();
    static ColorTheme getNightTheme();
    static ColorTheme getRainTheme();
    static ColorTheme getSnowTheme();
    
private:
    // 内部渲染实现
    class IconRendererImpl;
    std::unique_ptr<IconRendererImpl> impl_;
    
    // 图标定义
    struct IconDefinition {
        std::string name;
        std::vector<std::string> svg_paths;
        std::vector<std::pair<double, double>> control_points;
        bool has_fill;
        bool has_stroke;
    };
    
    static const std::vector<IconDefinition> icon_definitions_;
    
    const IconDefinition* findIconDefinition(const std::string& name) const;
    std::vector<uint8_t> renderToBitmap(const IconDefinition& icon, 
                                       IconSize size,
                                       const ColorTheme& theme);
    std::string renderToSVG(const IconDefinition& icon,
                           const ColorTheme& theme);
};

#endif // ICON_RENDERER_H