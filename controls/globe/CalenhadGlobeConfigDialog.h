//
// Created by martin on 10/05/17.
//

#ifndef CALENHAD_CALENHADGLOBECONFIGDIALOG_H
#define CALENHAD_CALENHADGLOBECONFIGDIALOG_H


#include <QtWidgets/QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QTextEdit>
#include <qwt/qwt_slider.h>
#include <marble/MarbleGlobal.h>
#include "CalenhadGlobeConstants.h"


namespace calenhad {
    namespace legend {
        class Legend;
        class LegendEditor;
        class LegendManager;
        class LegendWidget;
    }
    namespace controls {
        namespace globe {
            class CalenhadGlobe;

            class CalenhadGlobeConfigDialog : public QDialog {
            Q_OBJECT

            public:
                CalenhadGlobeConfigDialog (CalenhadGlobe* parent);

                virtual ~CalenhadGlobeConfigDialog ();

                void initialise ();

                bool overviewCheckState ();

                bool scaleCheckState ();

                bool zoomBarCheckState ();

                bool compassCheckState ();

                bool graticuleCheckState ();

                CalenhadGlobeDragMode dragMode ();

                CalenhadGlobeDoubleClickMode doubleClickMode ();

                double mouseSensitivity ();

                Marble::Projection selectedProjection ();

                calenhad::legend::Legend* selectedLegend ();

                void commitChanges ();

                calenhad::controls::globe::CoordinatesFormat coordinatesFormat ();

                calenhad::controls::globe::DatumFormat datumFormat ();

                void reject () override;

            protected:
                CalenhadGlobe* _parent;
                QCheckBox* _overviewCheck;
                QCheckBox* _scaleCheck;
                QCheckBox* _zoomBarCheck;
                QCheckBox* _compassCheck;
                QComboBox* _dragModeCombo;
                QComboBox* _doubleClickModeCombo;
                QwtSlider* _mouseSensitivitySlider;
                QCheckBox* _graticuleCheck;
                QComboBox* _projectionCombo;

                QWidget* _legendTab;
                calenhad::legend::LegendManager* _legendManager;

                QComboBox* _tooltipOptionCombo;
                QComboBox* _tooltipDatumCombo;

            };
        }
    }
}
#endif //CALENHAD_CALENHADGLOBECONFIGDIALOG_H