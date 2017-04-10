/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   geoutils.h
 * Author: martin
 *
 * Created on 30 September 2015, 17:17
 */

#ifndef GEOUTILS_H
#define GEOUTILS_H

// Converts degrees to radians.
#define degreesToRadians(angleDegrees) (angleDegrees * M_PI / 180.0)

// Converts radians to degrees.
#define radiansToDegrees(angleRadians) (angleRadians * 180.0 / M_PI)



#include <iostream>
#include <GeographicLib/Geocentric.hpp>
#include <QtCore/QString>
namespace geoutils {

    enum Units { Degrees, Radians };

    class IllegalGeoCoordinatesException : public std::runtime_error {
    public:
        IllegalGeoCoordinatesException (const std::string& msg) : runtime_error (msg) {}
    };


    class Geolocation {                // a point in spherical space
    public:
        Geolocation ();
        Geolocation (const double& newLat, const double& newLon, const unsigned& units = Units::Radians);
        Geolocation (const Geolocation& other);
        bool operator== (const Geolocation& other) const;
        Geolocation operator+ (const Geolocation& other);
        QString toString ();
        double latitude;
        double longitude;
        double latDegrees;
        double lonDegrees;
    };

    class Cartesian {                  // a point in three-dimensional space
    public:
        Cartesian();
        Cartesian (const double& newx, const double& newy, const double& newz);
        Cartesian (const Cartesian& other);
        double x, y, z;
        bool operator== (const Cartesian& other) const;
        Cartesian operator+ (const Cartesian& other);
        Cartesian operator- (const Cartesian& other);
    };


    class Math {
    public:
        static Geolocation toGeolocation (const Cartesian& c);
        static Cartesian toCartesian (const Geolocation& g);
        static double azimuth (const Geolocation& from, const Geolocation& unto);
        static void normalise (Cartesian& c);
        static double distSquared (const Cartesian& a, const Cartesian& b);
        static bool isNearer (const Cartesian& a, const Cartesian& b, const Cartesian& c);


    };
} // namespace Geoutils
#endif /* GEOUTILS_H */
