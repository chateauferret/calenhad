//
// Created by martin on 27/09/17.
//

#include <QtWidgets/QWidget>
#include <QMouseEvent>
#include "LegendEditorSliderPanel.h"
#include "LegendEditorSlider.h"
#include "LegendEditorScale.h"
#include "LegendEditor.h"
#include "../../legend/Legend.h"

using namespace calenhad::controls::legend;
using namespace calenhad::legend;

LegendEditorSliderPanel::LegendEditorSliderPanel (QWidget* parent) : QWidget (parent), _activeSlider (-1) {
    setContentsMargins (0, 0, 0, 0);
    setMouseTracking (true);
}

void LegendEditorSliderPanel::mousePressEvent (QMouseEvent* e) {
    if (e -> button () == Qt::LeftButton) {
        for (int i = 0; i < _editor->_sliders.size(); i++) {
            QRect srec = _editor->_sliders[i] -> geometry ();
            if (srec.contains (e -> pos())) {
                // only allow an entry to be dragged about if its key is a literal value, so as not to destroy expressions
                if (! (_editor -> _legend -> entries().at (i).isComputed())) {
                    _activeSlider = i;
                    _editor -> _sliders[i] -> setCursor (Qt::ClosedHandCursor);
                }
                break;
            }
        }
    }
}

void LegendEditorSliderPanel::mouseMoveEvent (QMouseEvent* e) {
    if (_activeSlider >= 0) {
        QRect crec = contentsRect();

        qreal pos;
        if (_editor->_orientation == Qt::Horizontal) {
            crec.adjust (_editor -> bspace_, 0, -_editor -> bspace_, 0);
            pos = 1.0 * (e -> pos().x() - _editor -> bspace_) / (crec.width());
        } else {
            crec.adjust (0, _editor -> bspace_, 0, -_editor -> bspace_);
            pos = 1.0 * (e -> pos().y() - _editor -> bspace_) / (crec.height());
        }


        if (crec.contains (e -> pos()))  {
            _editor -> _sliders[_activeSlider] -> assignCursor();
            if (_editor -> _orientation == Qt::Horizontal) {
                _editor -> _sliders[_activeSlider] -> move (e -> pos().x(), 0);
            } else {
                _editor -> _sliders[_activeSlider] -> move (0, e -> pos().y());
            }

            _editor -> updateKey (_editor->_sliders[_activeSlider]);
            qSort (_editor -> _sliders.begin (), _editor -> _sliders.end(), LegendEditor::SliderSort);
            if (_editor -> _slideUpdate) { _editor -> updateRamp(); }
        } else {
            _editor -> _sliders[_activeSlider] -> setCursor (Qt::ForbiddenCursor);
        }
    }
}

void LegendEditorSliderPanel::mouseReleaseEvent (QMouseEvent* e) {
    if (e->button () == Qt::LeftButton) {
        if (_activeSlider >= 0) {
            if (rect().contains (e -> pos())) {
                _editor ->_sliders[_activeSlider] -> assignCursor();
            } else {
                // delete the slider if it was dragged off the panel and dropped
                _editor -> deleteSlider (_activeSlider);
            }
        }
        _activeSlider = -1;
        _editor -> updateRamp ();
    }
}

void LegendEditorSliderPanel::mouseDoubleClickEvent (QMouseEvent* e) {
    if (e->button () == Qt::LeftButton) {
        int index = -1;
        for (int i = 0; i < _editor->_sliders.size (); i++) {
            QRect srec = _editor->_sliders[i] -> geometry();
            if (srec.contains (e -> pos(), true)) {
                index = i;
                break;
            }
        }
        if (index >= 0) {
            if (_editor -> _dialog) {
                _editor -> setSlider (index, _editor -> _legend -> entries ().at (index).key(), _editor -> _legend -> entries ().at (index).color());
                _editor -> _dialog -> setColor (_editor -> _legend -> entries ().at (index).color());
                _editor -> _dialog -> setIndex (_editor -> _legend -> entries ().at (index).key ());
                _editor -> _dialog -> preventDelete (_editor -> _legend -> entries().size() < 3);
                _editor -> _dialog -> show();
                connect (_editor -> _dialog, &QDialog::accepted, this, [=] () {
                    QColor color = _editor -> _dialog -> color();
                    QString key = _editor -> _dialog -> index();
                    LegendEntry entry = _editor -> _legend -> entries().at (index);
                    if (_editor -> _dialog -> isDelete()) {
                        _editor -> deleteSlider (index);
                    } else {
                        _editor -> setSlider (index, key, color);
                        _editor -> updatePos (_editor ->_sliders[index]);
                    }
                });
            }
        }
    }
}
