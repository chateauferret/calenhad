//
// Created by martin on 26/07/17.
//

#ifndef CALENHAD_EXPRESSIONEDIT_H
#define CALENHAD_EXPRESSIONEDIT_H


#include <QtWidgets/QWidget>
#include <QtWidgets/QTextEdit>
#include <QLineEdit>
#include <QWidget>
#include "../controls/TextEdit.h"


namespace calenhad {
    namespace expressions {

        class ExpressionEdit : public calenhad::controls::TextEdit {
            Q_OBJECT
        public:
            ExpressionEdit (QLineEdit* shortBox);

            virtual ~ExpressionEdit ();

            void focusOutEvent (QFocusEvent* e) override;
            void showEvent (QShowEvent* e) override;
            // to do - look up variable values and keywords on request


        signals:

            void dismissed (QString text);



        protected:
            QLineEdit* _shortBox;

        };

        class ExpressionLineEdit : public QLineEdit {
        public:
            ExpressionLineEdit (QWidget* parent);
            void focusInEvent (QFocusEvent* e) override;
        };

    }
}


#endif //CALENHAD_EXPRESSIONEDIT_H
