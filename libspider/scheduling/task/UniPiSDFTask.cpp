/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
/* === Include(s) === */

#include <scheduling/task/UniPiSDFTask.h>
#include <scheduling/launcher/TaskLauncher.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Graph.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>

#include <archi/Platform.h>
#include <api/archi-api.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::UniPiSDFTask::UniPiSDFTask(pisdf::GraphFiring *handler, const pisdf::Vertex *vertex) :
        PiSDFTask(handler, vertex) {
    const auto lrtCount = archi::platform()->LRTCount();
    syncInfoArray_ = spider::make_unique(make_n<u32, StackID::SCHEDULE>(lrtCount, UINT32_MAX));
}

void spider::sched::UniPiSDFTask::reset() {
    const auto lrtCount = archi::platform()->LRTCount();
    std::fill(syncInfoArray_.get(), syncInfoArray_.get() + lrtCount, UINT32_MAX);
    endTime_ = 0;
    jobExecIx_ = UINT32_MAX;
    mappedPEIx_ = UINT32_MAX;
    state_ = TaskState::NOT_SCHEDULABLE;
}

const spider::PE *spider::sched::UniPiSDFTask::mappedPe() const {
    return archi::platform()->peFromVirtualIx(mappedPEIx_);
}

void spider::sched::UniPiSDFTask::setMappedPE(const PE *pe) {
    mappedPEIx_ = static_cast<u32>(pe->virtualIx());
}
