//
// Created by martin on 27/11/16.
//

#include "QConstModule.h"
#include "../pipeline/ModuleFactory.h"
#include "QNode.h"
#include "../pipeline/CalenhadModel.h"
#include "../nodeedit/Calenhad.h"
#include "../preferences.h"

using namespace noise::module;
QConstModule::QConstModule (QWidget* parent) : QModule (new Const(), parent) {

}

void QConstModule::initialise() {
    QModule::initialise();
    _name = "New Constant Value";
    constValueSpin = noiseValueParamControl ("Constant value");
    connect (constValueSpin, SIGNAL (valueChanged (double)), this, SLOT (setConstValue (double)));
    _contentLayout -> addRow (tr ("Constant value"), constValueSpin);
    _isInitialised = true;
    emit initialised();
}

void QConstModule::setConstValue (double value) {
    module() -> SetConstValue (value);
    constValueSpin -> setValue (value);
    emit nodeChanged ("constValue", value);
}

double QConstModule::constValue () {
    return module() -> GetConstValue();
}

Const* QConstModule::module() {
    return dynamic_cast<Const*> (_module);
}

QConstModule* QConstModule::newInstance() {
    QConstModule* qm = new QConstModule();
    qm -> initialise();
    return qm;
}

QString QConstModule::moduleType () {
    return Calenhad::preferences -> calenhad_module_constant;
}

QConstModule* QConstModule::addCopy (CalenhadModel* model)  {
    QConstModule* qm = QConstModule::newInstance();
    if (qm) {
        qm -> setModel (model);
        qm -> setConstValue (constValue ());
    }
    return qm;
}

void QConstModule::inflate (const QDomElement& element, MessageFactory* messages) {
    QModule::inflate (element, messages);
    bool ok;

    double constValue = _model -> readParameter (element, "constValue").toDouble (&ok);
    if (ok) { setConstValue (constValue); }

}

void QConstModule::serialise (QDomDocument& doc, MessageFactory* messages) {
    QModule::serialise (doc, messages);
    _model -> writeParameter (_element, "constValue", QString::number (constValue()));
}