//
// Created by ferret on 07/08/2020.
//

#include "Cache.h"
#include "../nodeedit/Port.h"
#include "../nodeedit/Connection.h"

using namespace calenhad::module;
using namespace calenhad::graph;
using namespace calenhad::grid;
using namespace calenhad::nodeedit;
using namespace calenhad::module::algorithm;
using namespace calenhad::controls::globe;


Cache::Cache (const QString &nodeType) : StructuredGrid (nodeType), _algorithm (nullptr), _computed (false)  {

}

Cache::~Cache() {
    delete _algorithm;
}

void Cache::invalidate() {
    StructuredGrid::invalidate();
    if (isComplete()) {
        std::cout << name().toStdString() << " invalidated\n";
        _computed = false;
    }
}

void Cache::refresh() {
    std::cout << "Cache :: refresh - _computed " << _computed << "\n";
    //if (! _computed) {
        if (isComplete()) {
            if (! inputs().isEmpty()) {
                Port* p = inputs().first ();
                Module* source = p -> connections().first() -> otherEnd (p) -> owner ();

                // copy data from th
                // copy data source module into the input buffer
                source -> fetch (_buffer);
                _computed = true;
                CalenhadServices::compute() -> compute (this, _buffer);
                // we can now perform serial or parallel algorithms on the buffer
                if (_algorithm) {
                    _computed = _algorithm->execute (_buffer);
                }
            }
        }
    //}
}


void Cache::fetch (CubicSphere* buffer) {
    std::cout << "Cache :: fetch - buffer " << buffer << "\n";
    refresh();
    if (_computed) {
        buffer -> copy (_buffer);
    }
}

bool Cache::isComputed() {
    return _computed;
}

void Cache::setAlgorithm (Algorithm* algorithm) {
    _algorithm = algorithm;
}


