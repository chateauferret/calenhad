//
// Created by martin on 15/04/17.
//

#ifndef CALENHAD_SERVICELOCATOR_H
#define CALENHAD_SERVICELOCATOR_H

#include <QString>
#include <QDomDocument>
namespace calenhad {
    namespace pipeline {
        class ModuleFactory;
    }
    namespace expressions {
        class CalculatorService;
    }
    namespace preferences {
        class PreferencesService;
    }
    namespace notification {
        class QNotificationService;
    }
    namespace legend {
        class LegendService;
    }
    namespace controls {
        namespace globe {
            class ProjectionService;
            class StatisticsService;
        }
    }

    class CalenhadServices {

    public:
        static calenhad::preferences::PreferencesService* preferences ();

        static calenhad::notification::QNotificationService* messages ();

        static calenhad::legend::LegendService* legends ();

        static calenhad::controls::globe::ProjectionService* projections ();

        static calenhad::controls::globe::StatisticsService* statistics ();

        static calenhad::pipeline::ModuleFactory* modules ();

        static calenhad::expressions::CalculatorService* calculator ();

        static void providePreferences (calenhad::preferences::PreferencesService* service);

        static void provideMessages (calenhad::notification::QNotificationService* service);

        static void provideLegends (calenhad::legend::LegendService* service);

        static void provideProjections (calenhad::controls::globe::ProjectionService* service);

        static bool readXml (const QString& fname, QDomDocument& doc);

        static void provideCalculator (calenhad::expressions::CalculatorService* calculator);

    private:
        static calenhad::preferences::PreferencesService* _preferences;
        static calenhad::notification::QNotificationService* _messages;
        static calenhad::legend::LegendService* _legends;
        static calenhad::controls::globe::ProjectionService* _projections;
        static calenhad::controls::globe::StatisticsService* _statistics;
        static calenhad::pipeline::ModuleFactory* _modules;
        static calenhad::expressions::CalculatorService* _calculator;

    };
}


#endif //CALENHAD_SERVICELOCATOR_H
