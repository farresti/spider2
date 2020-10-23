/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <scheduling/scheduler/srdag-based/GreedyScheduler.h>
#include <scheduling/task/SRDAGTask.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/srdag/SRDAGVertex.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::vector<spider::srdag::Vertex *> spider::sched::GreedyScheduler::schedule(const srdag::Graph *graph) {
    tasks_.clear();
    auto result = factory::vector<srdag::Vertex *>(StackID::SCHEDULE);
    for (auto &vertex : graph->vertices()) {
        if (vertex->executable()) {
            evaluate(vertex.get(), result);
        }
    }
    return result;
}

/* === Private method(s) implementation === */

bool spider::sched::GreedyScheduler::evaluate(srdag::Vertex *vertex, spider::vector<srdag::Vertex *> &result) {
    auto schedulable = true;
    if (vertex->scheduleTaskIx() == SIZE_MAX) {
        for (const auto *edge : vertex->inputEdges()) {
            if (!edge->sourceRateValue()) {
                continue;
            } else if (!edge->source() || !edge->source()->executable()) {
                return false;
            }
            schedulable &= evaluate(edge->source(), result);
        }
        if (schedulable) {
            vertex->setScheduleTaskIx(result.size());
            result.emplace_back(vertex);
        }
    }
    return schedulable;
}

#endif