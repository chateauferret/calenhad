//
// Created by martin on 20/07/17.
//

#include "../messages/QNotificationHost.h"
#include "VariablesDialog.h"
#include <src/CalenhadServices.h>
#include <QVBoxLayout>
#include <QPushButton>
#include "../preferences/PreferencesService.h"
#include <QHeaderView>
#include <QtWidgets/QMessageBox>
#include <../messages/QNotificationHost.h>

using namespace calenhad::expressions;
using namespace calenhad::notification;

VariablesDialog::VariablesDialog() : QDialog(), _table (new QTableWidget()), _dirty (false) {
    _table -> setSelectionMode (QTableWidget::SelectionMode::SingleSelection);
    _table -> horizontalHeader () -> setSectionResizeMode (QHeaderView::ResizeMode::Stretch);
    setLayout (new QVBoxLayout());
    layout() -> addWidget (_table);

    _deleteButton = new QPushButton (this);
    _deleteButton -> setText ("Delete");
    _deleteButton -> setFixedWidth (100);
    connect (_deleteButton, &QPushButton::pressed, this, &VariablesDialog::deleteSelected);

    _insertButton = new QPushButton (this);
    _insertButton -> setText ("Insert");
    _insertButton -> setFixedWidth (100);
    connect (_insertButton, &QPushButton::pressed, this, &VariablesDialog::insertItem);

    connect (_table, &QTableWidget::itemSelectionChanged, this, &VariablesDialog::selectionChanged);

    QWidget* toolPanel = new QWidget (this);
    toolPanel -> setLayout (new QHBoxLayout());
    toolPanel -> layout() -> addWidget (_insertButton);
    toolPanel -> layout() -> addWidget (_deleteButton);
    layout() -> addWidget (toolPanel);
    _buttonBox = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset);
    connect (_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect (_buttonBox, &QDialogButtonBox::accepted, this, &VariablesDialog::commit);
    connect (_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect (_buttonBox, &QDialogButtonBox::rejected, this, &VariablesDialog::rollback);
    _rollbackButton = _buttonBox -> button (QDialogButtonBox::Reset);
    connect (_rollbackButton, &QPushButton::pressed, this, &VariablesDialog::rollback);
    _rollbackButton -> setEnabled (false);

    layout() -> addWidget (_buttonBox);

    // trigger validation when changing content
    connect (_table, &QTableWidget::itemChanged, this, &VariablesDialog::validateContent);
    resize (400, 300);
}


VariablesDialog::~VariablesDialog () {

}

void VariablesDialog::showEvent (QShowEvent* e) {
    _oldVariables = CalenhadServices::calculator() -> variables();
    // populate the table with the variables currently held by the service
    _dirty = false;
    rollback();
}

void VariablesDialog::deleteSelected() {
    if (_table -> currentRow() != -1) {
       _table -> removeRow (_table -> currentRow());
        _dirty = true;
    }
}

void VariablesDialog::insertItem() {
    int i = _table -> selectedItems().isEmpty() ? 0 : _table -> selectedItems().first() -> row();
    _table -> insertRow (i);
    QTableWidgetItem* item = new QTableWidgetItem ("New_variable");
    QTableWidgetItem* value = new QTableWidgetItem (QString::number ((double) 0.0));
    QTableWidgetItem* notes = new QTableWidgetItem ("");
    _table -> setItem (i, 0, item);
    _table -> setItem (i, 1, value);
    _table -> setItem (i, 2, notes);
    _table -> itemActivated (item);
    _table -> setItemSelected (item, true);
    _table -> editItem (item);
    _dirty = true;
}

void VariablesDialog::selectionChanged() {
    _deleteButton -> setEnabled (! _table -> selectedItems().isEmpty());
    for (QTableWidgetItem* item : _table -> selectedItems()) {
        _table -> selectRow (item -> row ());
        _table -> setCurrentItem (item);
    }
}

void VariablesDialog::resizeEvent (QResizeEvent* e) {
    setupColumns();
}

void VariablesDialog::commit () {
    for (int i = 0; i < _table -> rowCount(); i++) {
        QString name = _table -> item (i, 0) -> text();
        QString notes = _table -> item (i, 2) -> text();
        double v = value (_table -> item (i, 1));
        CalenhadServices::calculator() -> updateVariable (name, notes, v, false);
    }
    CalenhadServices::calculator() -> publish();
}

double VariablesDialog::value (QTableWidgetItem* item) {
    bool ok;
    double value = item -> text().toDouble (&ok);
    if (ok) {
        return value;
    } else {
        return 0.0;
    }
}

void VariablesDialog::rollback() {
    int ret = QMessageBox::Discard;
    if (_dirty) {
        QMessageBox msgBox;
        msgBox.setText ("Discard changes?");
        msgBox.setStandardButtons (QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton (QMessageBox::Cancel);
        ret = msgBox.exec ();
        if (ret == QMessageBox::Discard) {
            CalenhadServices::messages() -> message ("Cancelled", "Changes to variables were rolled back", NotificationStyle::InfoNotification);
        }
    }

    if (ret == QMessageBox::Discard) {
        _table->clear ();
        setupColumns ();

        _table->setRowCount (_oldVariables.size());
        int row = 0;

        for (QString key : _oldVariables.keys()) {
            QTableWidgetItem* item = new QTableWidgetItem (key);
            CalenhadVariable* cv = _oldVariables.value (key);
            QTableWidgetItem* value = new QTableWidgetItem (QString::number (cv -> _value));
            QTableWidgetItem* notes = new QTableWidgetItem (cv -> _notes);
            _table->setItem (row, 0, item);
            _table->setItem (row, 1, value);
            _table->setItem (row, 2, notes);
            row++;
        }

        QStringList headers;
        headers << "Name" << "Value" << "Notes";
        _table->setColumnCount (3);
        _table->setHorizontalHeaderLabels (headers);
    }
    _dirty = false;
};

void VariablesDialog::setupColumns() {

    QStringList headers;
    headers << "Name" << "Value" << "Notes";
    _table -> setColumnCount (3);
    _table -> setHorizontalHeaderLabels (headers);
    _table -> setColumnWidth (0, _table -> width() * 0.25);
    _table -> setColumnWidth (1, _table -> width() * 0.15);
    _table -> setColumnWidth (2, _table -> width() - (_table -> columnWidth (0) + _table -> columnWidth (1)));
}

void VariablesDialog::validateContent() {
    bool result = true;
    for (int i = 0; i < _table -> rowCount(); i++) {
        if (! validate (i)) {
            result = false;
        }
    }
    _dirty = true;
    _buttonBox -> button (QDialogButtonBox::Ok) -> setEnabled (result);

}

bool VariablesDialog::validate (int row) {
    _rollbackButton -> setEnabled (true);
    QString nameTooltip = "";
    QString valueTooltip = "";
    bool nameOk = true;
    bool valueOk = true;

    // validate variable name
    if (_table -> item (row, 0)) {
        QString name = _table->item (row, 0)->text ();
        // name must meet requirements of the calculator service
        nameOk = CalenhadServices::calculator() -> validateVariableName (name, nameTooltip);

        if (nameOk) {
            // name must be unique
            for (int i = 0; i < _table->rowCount (); i++) {
                if (_table->item (i, 0)) {
                    if (i != row && name.toLower () == _table->item (i, 0)->text ().toLower ()) {
                        nameTooltip += "Name is not unique\n";
                        nameOk = false;
                        break;
                    }
                }
            }
        }
        _table -> blockSignals (true);
        _table -> item (row, 0) -> setToolTip (nameTooltip);
        _table -> item (row, 0) -> setTextColor (nameOk ? CalenhadServices::preferences() -> calenhad_variables_color_normal : CalenhadServices::preferences() -> calenhad_variables_color_error);
        _table -> blockSignals (false);
    } else {
        nameTooltip += "Name is required\n";
        nameOk = false;
    }

    // validate variable value
    if (_table -> item (row, 1)) {
        QString value = _table -> item (row, 1) -> text();
        valueOk = CalenhadServices::calculator() -> validateVariableValue (value, valueTooltip);
        _table -> blockSignals (true);
        _table -> item (row, 1) -> setToolTip (valueTooltip);
        _table -> item (row, 1) -> setTextColor (valueOk ? CalenhadServices::preferences() -> calenhad_variables_color_normal : CalenhadServices::preferences() -> calenhad_variables_color_error);
        _table -> blockSignals (false);
    } else {
        valueTooltip += "Value is required\n";
        valueOk = false;
    }

    return (valueOk && nameOk);

}
