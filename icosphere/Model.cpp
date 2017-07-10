
#include "Model.h"

using namespace icosphere;

    Model::Model (const unsigned& depth) : _depth (depth) {

    }

    Model::~Model() {

    }

    double Model::distance (const Geolocation& from, const Geolocation& unto) const {
        double d;
        _gd -> Inverse (from.latitude, from.longitude, unto.latitude, unto.longitude, d);
        return d;
    }

    double Model::loxodrome (const Geolocation& from, const Geolocation& unto) {
        double azimuth;
        double length;
        _rhumb -> Inverse (from.latitude, from.longitude, unto.latitude, unto.longitude, azimuth, length);
        return azimuth;
    }

