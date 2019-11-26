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
#ifndef SPIDER2_CONTAINERS_H
#define SPIDER2_CONTAINERS_H

/* === Includes === */

#include <set>
#include <map>
#include <deque>
#include <queue>
#include <stack>
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <unordered_map>
#include <memory/STLAllocator.h>
#include <spider-api/config.h>

/* === Namespace === */

namespace spider {

    template<class T>
    using vector = std::vector<T, spider::Allocator<T>>;

    template<class T>
    inline spider::vector<T> make_vector(size_t size, StackID stack = StackID::GENERAL) {
        return spider::vector<T>(size, spider::Allocator<T>(stack));
    }

    template<class T>
    inline spider::vector<T> make_vector(StackID stack = StackID::GENERAL) {
        return spider::vector<T>(spider::Allocator<T>(stack));
    }

    template<class T>
    using deque = std::deque<T, spider::Allocator<T>>;

    template<class T>
    using queue = std::queue<T, spider::deque<T>>;

    template<class Key>
    using unordered_set = std::unordered_set<Key, std::hash<Key>, std::equal_to<Key>, spider::Allocator<Key>>;

    template<class Key>
    using set = std::set<Key, std::less<Key>, spider::Allocator<Key>>;

    template<class Key, class T>
    using unordered_map = std::unordered_map<Key,
            T,
            std::hash<Key>,
            std::equal_to<Key>,
            spider::Allocator<std::pair<const Key, T>>>;

    template<class Key, class T>
    using map = std::map<Key, T, std::less<Key>, spider::Allocator<std::pair<const Key, T>>>;

    template<class T>
    using forward_list = std::forward_list<T, spider::Allocator<T>>;

    template<class T, class Container = spider::deque<T>>
    using stack = std::stack<T, Container>;

    using string = std::basic_string<char, std::char_traits<char>, spider::Allocator<char>>;
}

#endif //SPIDER2_CONTAINERS_H
