//
// Created by martin on 26/11/16.
//

#include <libnoise/module/modulebase.h>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QDoubleSpinBox>
#include "QCylindersModule.h"
#include "../pipeline/ModuleFactory.h"
#include "QNode.h"
#include "../pipeline/CalenhadModel.h"
#include "../nodeedit/Calenhad.h"
#include "../preferences.h"

QCylindersModule::QCylindersModule (QWidget* parent) : QModule (new noise::module::Cylinders(), parent) {

}

QCylindersModule::~QCylindersModule () {

}

void QCylindersModule::initialise() {
    QModule::initialise();
    _name = "New Cylinders";
    frequencySpin = logParameterControl ("Constant value");
    connect (frequencySpin, SIGNAL (valueChanged (double)), this, SLOT (setFrequency (double)));
    _contentLayout -> addRow (tr ("Frequency"), frequencySpin);
    _isInitialised = true;
    emit initialised();
}

double QCylindersModule::frequency() {
    return module() -> GetFrequency ();
}

void QCylindersModule::setFrequency (double value) {
    module() -> SetFrequency (value);
    emit (nodeChanged ("frequency", value));
    frequencySpin -> setValue (value);
}


Cylinders* QCylindersModule::module () {
    Cylinders* cylinders = dynamic_cast<Cylinders*> (_module);
    return cylinders;
}

QCylindersModule* QCylindersModule::newInstance () {
    QCylindersModule* qm = new QCylindersModule();
    qm -> initialise();
    return qm;
}

QString QCylindersModule::moduleType () {
    return Calenhad::preferences -> calenhad_module_cylinders;
}

QCylindersModule* QCylindersModule::addCopy (CalenhadModel* model)  {
    QCylindersModule* qm = QCylindersModule::newInstance();
    if (qm) {
        qm -> setModel (model);
        qm -> setFrequency (frequency());
    }
    return qm;
}


void QCylindersModule::inflate (const QDomElement& element, MessageFactory* messages) {
    QModule::inflate (element, messages);
    bool ok;

    double frequency = _model -> readParameter (element, "frequency").toDouble (&ok);
    if (ok) { setFrequency (frequency); }
}

void QCylindersModule::serialise (QDomDocument& doc, MessageFactory* messages) {
    QModule::serialise (doc, messages);
    _model -> writeParameter (_element, "frequency", QString::number (frequency()));
}