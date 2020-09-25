/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_GRAPHHANDLER_H
#define SPIDER2_GRAPHHANDLER_H

/* === Include(s) === */

#include <common/Types.h>
#include <memory/unique_ptr.h>
#include <containers/vector.h>

namespace spider {

    namespace pisdf {
        class Graph;

        class Param;
    }
    namespace srless {

        class FiringHandler;

        /* === Class definition === */

        class GraphHandler {
        public:
            GraphHandler(const pisdf::Graph *graph,
                         const spider::vector<std::shared_ptr<pisdf::Param>> &params,
                         u32 repetitionCount);

            GraphHandler(GraphHandler &&) = default;

            GraphHandler(const GraphHandler &) = default;

            GraphHandler &operator=(GraphHandler &&) = default;

            GraphHandler &operator=(const GraphHandler &) = default;

            ~GraphHandler() = default;

            /* === Method(s) === */

            /* === Getter(s) === */

            const pisdf::Graph *graph() const { return graph_; }

            u32 repetitionCount() const;

            inline bool isStatic() const { return static_; }

            inline const spider::vector<FiringHandler> &firings() const { return firings_; }

            inline spider::vector<FiringHandler> &firings() { return firings_; }

            /* === Setter(s) === */

        private:
            spider::vector<srless::FiringHandler> firings_;
            const pisdf::Graph *graph_;
            u32 repetitionCount_;
            bool static_;

            /* === private method(s) === */
        };
    }
}

#endif //SPIDER2_GRAPHHANDLER_H
