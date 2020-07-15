//
// Created by martin on 08/08/17.
//

#include <src/CalenhadServices.h>
#include <iostream>
#include "nodeedit/Port.h"
#include "preferences/preferences.h"
#include "graph.h"
#include "module/Module.h"
#include "nodeedit/NodeBlock.h"
#include "nodeedit/Connection.h"
#include "../pipeline/ModuleFactory.h"
#include "../mapping/Curve.h"
#include "exprtk/Calculator.h"
#include <QList>
#include <module/AltitudeMap.h>
#include <module/Convolution.h>
#include "../messages/QNotificationHost.h"
#include "../legend//Legend.h"

using namespace calenhad;
using namespace calenhad::nodeedit;
using namespace calenhad::module;
using namespace calenhad::preferences;
using namespace calenhad::graph;
using namespace calenhad::mapping;
using namespace calenhad::legend;
using namespace grid;
using namespace calenhad::expressions;
using namespace calenhad::controls::altitudemap;
using namespace exprtk;
/*
Graph::Graph (const QString& xml, const QString& nodeName) : _xml (xml), _nodeName (nodeName), _colorMapBuffer (nullptr), _parser (new parser<double>()), _rasterId = 0; {
    _doc.setContent (_xml);

}

Graph::Graph (const QDomDocument& doc, const QString& nodeName): _doc (doc), _nodeName (nodeName), _colorMapBuffer (nullptr), _parser (new parser<double>()) {

}
*/

Graph::Graph (calenhad::module::Module* module) : _model (module -> model()), _module (module), _nodeName (module -> name()), _colorMapBuffer (nullptr), _parser (new parser<double>()), _rasterId (0) {

}


Graph::~Graph () {
    if (_colorMapBuffer) { delete (_colorMapBuffer); }
    delete _parser;
}

QString Graph::glsl() {
    _code =  glsl (_module);
    std::cout << "Module " << _module -> name().toStdString () << "\n";

    if (_code != QString::null) {
        _code.append ("\n\nfloat value (vec3 cartesian, vec2 geolocation) {\n");
        _code.append ("    return _" + _nodeName + " (cartesian, geolocation);\n");
        _code.append ("}\n");
    }
    std::cout << _code.toStdString () << "\n\n";
    return _code;
}

QString Graph::glsl (Module* module) {
    if (module -> isComplete()) {
        QString name = module -> name ();
        if (! _code.contains ("float _" + name)) {
            // Compile any antecedent modules recurisvely
            for (unsigned p: module->inputs ().keys ()) {
                Port* port = module->inputs ().value (p);
                if (!port->connections ().empty ()) {
                    for (Connection* c : port->connections ()) {
                        Port* p = c->otherEnd (port);
                        if (p) {
                            Node* other = p->owner ();
                            if (other && other != module) {
                                Module* qm = static_cast<Module*> (other);
                                if (qm) {
                                    _code = glsl (qm);
                                }
                            }
                        }
                    }
                }
            }

            Module* qm = dynamic_cast<Module*> (module);
            if (qm) {

                // Find out what type of module this node wants and construct glsl for it
                QString type = qm->nodeType ();

                // if it's an altitude map, compile the decision tree
                if (type == CalenhadServices::preferences ()->calenhad_module_altitudemap) {
                    AltitudeMap* am = dynamic_cast<AltitudeMap*> (qm);
                    QVector<AltitudeMapping> entries = am->entries ();

                    // input is below the bottom of the range
                    _code += "float _" + name + " (vec3 c, vec2 g) {\n";
                    _code += "  float value = %0 ;\n";
                    _code += "  if (value < " + QString::number (entries.first ().x ()) + ") { return " + QString::number (entries.first ().y ()) + "; }\n";

                    for (int j = 0; j < entries.size (); j++) {
                        // Do we need to sort the entries?
                        QString mapFunction;

                        // compose the mapping function
                        // spline function
                        if (am->curveFunction () == "spline") {
                            double x[4], y[4];

                            for (int i = 0; i < 4; i++) {
                                int k = std::max (j + i - 2, 0);
                                k = std::min (k, entries.size () - 1);
                                x[i] = entries.at (k).x ();
                                y[i] = entries.at (k).y ();
                            }

                            // Compute the alpha value used for cubic interpolation.
                            mapFunction += "        float alpha = ((value - " + QString::number (x[1]) + ") / " + QString::number (x[2] - x[1]) + ");\n";
                            mapFunction += "        return cubicInterpolate (" +
                                           QString::number (y[0]) + ", " +
                                           QString::number (y[1]) + ", " +
                                           QString::number (y[2]) + ", " +
                                           QString::number (y[3]) + ", alpha);";
                            _code += "  if (value > " + QString::number (x[1]) + " && value <= " + QString::number (x[2]) + ") {\n" + mapFunction + "\n   }\n";
                        }

                        // terrace function
                        if (am -> curveFunction () == "terrace") {
                            double x[2], y[2];
                            for (int i = 0; i < 2; i++) {
                                int k = std::max (j + i - 1, 0);
                                k = std::min (k, entries.size () - 1);
                                x[i] = entries.at (k).x ();
                                y[i] = entries.at (k).y ();
                            }

                            // Compute the alpha value used for cubic interpolation.
                            mapFunction += "        float alpha = ((value - " + QString::number (x[0]) + ") / " + QString::number (x[1] - x[0]) + ");\n";
                            if (am->isFunctionInverted ()) { mapFunction += "        alpha = 1 - alpha;\n"; }
                            mapFunction += "        alpha *= alpha;\n";
                            mapFunction += "        return mix ( float(" +
                                           QString::number (am->isFunctionInverted () ? y[1] : y[0]) + "), float (" +
                                           QString::number (am->isFunctionInverted () ? y[0] : y[1]) + "), " +
                                           "alpha);";
                            _code += "  if (value > " + QString::number (x[0]) + " && value <= " + QString::number (x[1]) + ") {\n" + mapFunction + "\n   }\n";
                        }
                    }

                    // input is beyond the top of the range
                    _code += "  if (value > " + QString::number (entries.last ().x ()) + ") { return " + QString::number (entries.last ().y ()) + "; }\n";
                    _code += "}\n";
                } else {
                    QString func = qm -> glsl ();;
                    _code.append ("float _" + name + " (vec3 c, vec2 g) { return " + func);
                    //_code.append ("; }\n");
                }

                // if it's a convolution module, compile and queue the module for raster upload
                if (type == CalenhadServices::preferences()->calenhad_module_raster) {
                    Convolution* rm = (Convolution*) qm;
                    Bounds bounds = Bounds();
                    QString boundsCode;

                    _rasters.insert (_rasterId++, rm);

                    // replace the bounds marker with the module's declared bounds
                    boundsCode.append ("vec2 (" + QString::number (bounds.west ()) + ", " + QString::number (bounds.north ()) + ")");
                    boundsCode.append (", ");
                    boundsCode.append ("vec2 (" + QString::number (bounds.east ()) + ", " + QString::number (bounds.south ()) + ")");
                    _code.replace ("%bounds", boundsCode);
                    _code.append ("; }\n");
                }

                // replace the input module markers with their names referencing their member variables in glsl
                int i = 0;
                for (Port* port : qm->inputs ()) {
                    QString index = QString::number (i++);
                    if (port->connections ().isEmpty ()) {
                        _code.replace ("%" + index, QString::number (qm -> parameterValue (port ->portName ())));
                    } else {
                        Node* other = port->connections ()[0]->otherEnd (port)->owner ();
                        QString source = other->name ();
                        _code.replace ("%" + index, "_" + source + " (c, g)");    // "%0" is shorthand for "$0 (c)"
                        _code.replace ("$" + index, "_" + source);
                    }
                }

                // fill in attribute values by looking for words beginning with % and replacing them with the parameter values from the XML
                for (QString param : CalenhadServices::modules() -> paramNames ()) {
                    if (qm->parameters ().contains (param)) {
                        _code.replace ("%" + param, QString::number (qm->parameterValue (param)));
                    }
                }

                // populate index number for a raster
                Convolution* rm = dynamic_cast<Convolution*> (module);
                if (rm) {
                    _code.replace ("%index", QString::number (_rasterId - 1));
                }
            }
        }
        return _code;
    } else {
        return QString::null;
    }
}


int Graph::rasterCount() const {
    return _rasterId;
}

QImage* Graph::raster (const int& index) {
    return _rasters.value (index) -> raster();
}

