//
// Created by martin on 10/10/17.
//

#ifndef CALENHAD_GLOBESCALEWIDGET_H
#define CALENHAD_GLOBESCALEWIDGET_H

#include <QtWidgets/QWidget>
#include <qwt/qwt_scale_widget.h>
#include <qwt/qwt_scale_engine.h>
#include <GeographicLib/Geodesic.hpp>
#include "../../preferences/PreferencesService.h"
#include "../../CalenhadServices.h"

namespace calenhad {
    class CalenhadServices;
    namespace mapping {
        class CalenhadMapWidget;
    }
    namespace controls {
        namespace globe {

            class GlobeScaleWidget : public QwtScaleWidget {
            public:
                explicit GlobeScaleWidget (calenhad::mapping::CalenhadMapWidget* _globe, QWidget* parent = nullptr, const double& radius = 6440000);
                ~GlobeScaleWidget() override;
                void setOrientation (Qt::Orientation orientation);
            protected:
                QwtScaleDraw* _draw;
                QwtLinearScaleEngine* _scaleEngine;

                void paintEvent (QPaintEvent* e) override;

                Qt::Orientation _orientation;
                calenhad::mapping::CalenhadMapWidget*  _globe;
                double _radius;
                GeographicLib::Geodesic* _geodesic;
                double _metresPerUnit;
            };
        }
    }
}


#endif //CALENHAD_GLOBESCALEWIDGET_H
