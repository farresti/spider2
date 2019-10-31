/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
 *
 * Spider is a dataflow based runtime used to execute dynamic PiSDF
 * applications. The Preesm tool may be used to design PiSDF applications.
 *
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */
#ifndef SPIDER2_PARSERFUNCTIONS_H
#define SPIDER2_PARSERFUNCTIONS_H

/* === Includes === */

#include <cstdint>
#include <cmath>

/* === Methods prototype === */

namespace Spider {
    inline double dummyEval(const double &, const double &) {
        return 0.;
    }

    inline double add(const double &a, const double &b) {
        return a + b;
    }

    inline double sub(const double &a, const double &b) {
        return a - b;
    }

    inline double mul(const double &a, const double &b) {
        return a * b;
    }

    inline double div(const double &a, const double &b) {
        return a / b;
    }

    inline double mod(const double &a, const double &b) {
        return static_cast<std::int64_t >(a) % static_cast<std::int64_t >(b);
    }

    inline double pow(const double &a, const double &b) {
        return std::pow(a, b);
    }

    inline double max(const double &a, const double &b) {
        return std::max(a, b);
    }

    inline double min(const double &a, const double &b) {
        return std::min(a, b);
    }

    inline double cos(const double &, const double &b) {
        return std::cos(b);
    }

    inline double sin(const double &, const double &b) {
        return std::sin(b);
    }

    inline double tan(const double &, const double &b) {
        return std::tan(b);
    }

    inline double exp(const double &, const double &b) {
        return std::exp(b);
    }

    inline double log(const double &, const double &b) {
        return std::log(b);
    }

    inline double log2(const double &, const double &b) {
        return std::log2(b);
    }

    inline double ceil(const double &, const double &b) {
        return std::ceil(b);
    }

    inline double floor(const double &, const double &b) {
        return std::floor(b);
    }

    inline double sqrt(const double &, const double &b) {
        return std::sqrt(b);
    }
}

#endif //SPIDER2_PARSERFUNCTIONS_H
