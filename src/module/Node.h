//
// Created by martin on 16/12/16.
//

#ifndef CALENHAD_QNODE_H
#define CALENHAD_QNODE_H


#include <QWidget>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QComboBox>
#include <QtCore/QSet>

#include <QtWidgets/QGraphicsItem>
#include <qwt/qwt_counter.h>
#include <QtXml/QDomElement>
#include <exprtk/ExpressionWidget.h>
#include "src/Serializable.h"
#include "ParamValidator.h"
#include <limits>
#include <QtWidgets/QDockWidget>

namespace calenhad {
    namespace pipeline {
        class CalenhadModel;
    }
    namespace notification {
        class QNotificationFactory;
    }
    namespace nodeedit {
        class Port;
        class NodeBlock;
        class NodeNameValidator;
    }
    namespace expressions {
        class ExpressionWidget;
    }
    namespace module {
        class Module;
        class NodeGroup;

        class Node : public QWidget, public Serializable {
        Q_OBJECT



        public:
            explicit Node (QString  nodeType, QWidget* parent = 0);


            enum {
                Type = QGraphicsItem::UserType + 6
            };

             ~Node() override;

            void inflate (const QDomElement& element) override;

            void serialize (QDomElement& element) override;

            virtual void setModel (calenhad::pipeline::CalenhadModel* model);

            virtual QString nodeType();

            virtual void setGroup (NodeGroup* group);

            NodeGroup* group ();

            virtual QGraphicsItem* handle();

            virtual Node* clone();

            QString name ();

            void setNotes (const QString& notes);

            QString notes ();


            void showEvent (QShowEvent* event) override;

            void closeEvent (QCloseEvent* event) override;

            calenhad::pipeline::CalenhadModel* model ();
            calenhad::expressions::ExpressionWidget* addParameter (const QString& label, const QString& name, const double& initial, ParamValidator* validator = new AcceptAnyRubbish(), QWidget* panel = 0);
            virtual bool hasParameters ();

            Q_PROPERTY (QString name READ name WRITE setName MEMBER _name NOTIFY nameChanged);
            Q_PROPERTY (QString notes READ notes WRITE setNotes MEMBER _notes NOTIFY notesChanged);

            virtual bool isComplete ();

            void propertyChangeRequested (const QString& p, const QVariant& value);

            QStringList parameters();

            void setParameter (const QString& label, const QString& value);
            QString parameter (const QString& label);

            virtual QGraphicsItem* makeHandle ();
            double parameterValue (const QString& name);
            //virtual void addDependentNodes();

            //QVector<Node*> dependants();
            //void showName (const bool& visible);
            //bool nameVisible();
            bool isModuleDetailVisible();

        public slots:
            virtual void showModuleDetail (const bool& visible);
            virtual void invalidate ();
            void setName (const QString& name);
            virtual void parameterChanged();
        signals:

            void nameChanged (const QString&);

            void notesChanged (const QString&);

            void nodeChanged ();

        protected:

            NodeGroup* _group;
            QWidget *_propertiesWidget;
            calenhad::nodeedit::NodeBlock* _block;
            QString _name;
            QString _notes;
            QLineEdit* _nameEdit;
            QTextEdit* _notesEdit;
            QToolBox* _expander;
            //QVector<Node*> _dependants;
            QWidget* _content;
            calenhad::pipeline::CalenhadModel* _model;

            QMap<QString, calenhad::expressions::ExpressionWidget*> _parameters;


            int addPanel (const QString& name, QWidget* widget);

            QFormLayout* _contentLayout;
            QDomElement _element;
            QDomDocument _document;
            QString _nodeType;
            static QString propertyName (const QString& text);

            void addContentPanel ();

            calenhad::nodeedit::NodeNameValidator* _validator;
            QPalette* _palette;


            QWidget* _topPanel;
            QDockWidget *_propertiesDock;
            QWidget* _about;
        };

    } // namespace module
}// namespace calenhad
#endif //CALENHAD_QNODE_H
