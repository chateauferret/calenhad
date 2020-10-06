//
// Created by martin on 05/04/17.
//

#ifndef CALENHAD_ALTITUDEMAPFITTER_H
#define CALENHAD_ALTITUDEMAPFITTER_H


#include <qwt/qwt_curve_fitter.h>

class QwtPlot;

namespace calenhad {
    namespace controls {
        namespace altitudemap {


            class AltitudeMapFitter : public QwtCurveFitter {

            public:
                AltitudeMapFitter (const bool& quartic);

                virtual ~AltitudeMapFitter ();

                QPolygonF fitCurve (const QPolygonF& points) const override;

                void setResolution (const int& resolution);

                int resolution ();

                void setInverted (const bool& inverted);

                bool isInverted () const;

                void attachPlot (QwtPlot* plot);

            protected:
                double getY (const double& x, const QPolygonF& points) const;

                int _resolution;
                bool _inverted;
                mutable double _min, _max;
                QwtPlot* _plot;

                bool _quartic;
            };
        }
    }
}

#endif //CALENHAD_ALTITUDEMAPFITTER_H
