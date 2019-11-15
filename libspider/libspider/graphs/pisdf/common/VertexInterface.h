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
#ifndef SPIDER2_VERTEXINTERFACE_H
#define SPIDER2_VERTEXINTERFACE_H

/* === Include(s) === */

#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/common/Visitor.h>

namespace Spider {
    namespace PiSDF {

        /* === Class definition === */

        template<class T>
        class VertexInterface : public Vertex {
        public:
            VertexInterface(std::string name,
                            VertexType type,
                            std::uint32_t edgeINCount,
                            std::uint32_t edgeOUTCount,
                            Graph *graph,
                            StackID stack) : Vertex(std::move(name),
                                                    type,
                                                    edgeINCount,
                                                    edgeOUTCount,
                                                    graph,
                                                    stack) { }

            ~VertexInterface() override = default;

            /* === Method(s) === */

            inline void visit(Visitor *visitor) {
                visitor->visit(static_cast<T *>(this));
            }

            /* === Getter(s) === */

            /* === Setter(s) === */

        private:

        };

        /* === Inline method(s) === */
    }
}


#endif //SPIDER2_VERTEXINTERFACE_H
