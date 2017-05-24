//
// Created by martin on 10/05/17.
//

#ifndef CALENHAD_CALENHADGLOBECONFIGDIALOG_H
#define CALENHAD_CALENHADGLOBECONFIGDIALOG_H


#include <QtWidgets/QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <qwt/qwt_slider.h>
#include "CalenhadGlobe.h"

class CalenhadGlobe;

class CalenhadGlobeConfigDialog : public QDialog {
    Q_OBJECT
public:
    CalenhadGlobeConfigDialog (CalenhadGlobe* parent);

    virtual ~CalenhadGlobeConfigDialog ();

    void initialise();
    bool overviewCheckState();
    bool scaleCheckState();
    bool zoomBarCheckState();
    bool compassCheckState();
    bool graticuleCheckState();
    CalenhadGlobeDragMode dragMode();
    CalenhadGlobeDoubleClickMode doubleClickMode();
    double mouseSensitivity();
    Projection selectedProjection();

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


};


#endif //CALENHAD_CALENHADGLOBECONFIGDIALOG_H