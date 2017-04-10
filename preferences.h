//
// Created by martin on 16/10/16.
//

#ifndef CALENHAD_PREFERENCES_H
#define CALENHAD_PREFERENCES_H
#include <QSettings>
#include <QFont>
#include <QString>
#include <QColor>

class Preferences {
public:
    Preferences();
    ~Preferences();
    void saveSettings();
    void loadSettings();
    QSettings* settings() { return _settings; }
    QString calenhad_stylesheet;
    QColor calenhad_handle_brush_color_selected;
    QColor calenhad_handle_brush_color_normal;
    QColor calenhad_handle_brush_color_bg;
    unsigned calenhad_port_radius;
    unsigned calenhad_port_margin;
    QColor calenhad_port_in_border_color;
    QColor calenhad_port_in_fill_color;
    QColor calenhad_port_out_border_color;
    QColor calenhad_port_out_fill_color;
    QColor calenhad_port_control_border_color;
    QColor calenhad_port_control_fill_color;
    unsigned calenhad_port_border_weight;
    QColor calenhad_port_text_color;
    QColor calenhad_connector_normal_color;
    unsigned calenhad_connector_normal_weight;
    QColor calenhad_connector_selected_color;
    unsigned calenhad_connector_selected_weight;
    QColor calenhad_connector_drawing_color;
    unsigned calenhad_connector_drawing_weight;
    QColor calenhad_connector_inactive_color;
    unsigned calenhad_connector_inactive_weight;
    QColor calenhad_port_in_fill_color_drop;
    QColor calenhad_port_in_border_color_drop;
    QColor calenhad_handle_text_color_selected;
    QColor calenhad_handle_text_color_normal;
    unsigned calenhad_altitudemap_deletemargin;

    // modules

    QString calenhad_module_perlin;
    QString calenhad_module_cylinders;
    QString calenhad_module_spheres;
    QString calenhad_module_exponent;
    QString calenhad_module_translate;
    QString calenhad_module_rotate;
    QString calenhad_module_clamp;
    QString calenhad_module_constant;
    QString calenhad_module_add;
    QString calenhad_module_abs;
    QString calenhad_module_blend;
    QString calenhad_module_cache;
    QString calenhad_module_checkerboard;
    QString calenhad_module_invert;
    QString calenhad_module_max;
    QString calenhad_module_min;
    QString calenhad_module_multiply;
    QString calenhad_module_power;
    QString calenhad_module_displace;
    QString calenhad_module_diff;
    QString calenhad_module_billow;
    QString calenhad_module_ridgedmulti;
    QString calenhad_module_scalebias;
    QString calenhad_module_select;
    QString calenhad_module_turbulence;
    QString calenhad_module_voronoi;
    QString calenhad_module_scalepoint;
    QString calenhad_module_icospheremap;
    QString calenhad_module_altitudemap;


private:
    QSettings* _settings;


};


#endif //CALENHAD_PREFERENCES_H