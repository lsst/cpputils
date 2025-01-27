/*
 * LSST Data Management System
 * See COPYRIGHT file at the top of the source tree.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */
#ifndef LSST_CPPUTILS_CACHE_H
#define LSST_CPPUTILS_CACHE_H

#include <vector>
#include <optional>
#include <utility>  // std::pair

#include "boost/multi_index_container.hpp"
#include "boost/multi_index/sequenced_index.hpp"
#include "boost/multi_index/hashed_index.hpp"
#include "boost/multi_index/composite_key.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/format.hpp"

#include "lsst/pex/exceptions.h"
#include "lsst/cpputils/CacheFwd.h"

//#define LSST_CACHE_DEBUG 1  // Define this variable to instrument for debugging


#ifdef LSST_CACHE_DEBUG
#include <iostream>
#include <fstream>
#include <typeinfo>
#include "lsst/cpputils/Demangle.h"
#endif

namespace lsst {
namespace cpputils {

/** Cache of most recently used values
 *
 * This object stores the most recent `maxElements` values, where `maxElements`
 * is set in the constructor. Objects (of type `Value`) are stored by a key (of
 * type `Key`; hence the need to provide a `KeyHash` and `KeyPred`), and the
 * class presents a dict-like interface. Objects may be added to (`add`) and
 * retrieved from (`operator[]`) the cache. For ease of use, an interface
 * (`operator()`) is also provided that will check the cache for an existing
 * key, and if the key is not present, generate it with a function provided by
 * the user.
 *
 * @note `Value` and `Key` must be copyable.
 *
 * @note This header (`Cache.h`) should generally only be included in source
 * files, not other header files, because you probably don't want all of the
 * `boost::multi_index` includes in your header. We suggest you se the
 * `CacheFwd.h` file in your header instead, and hold the `Cache` as a
 * `std::unique_ptr`.
 *
 * @note Python bindings (for pybind11) are available in
 * `lsst/cpputils/python/Cache.h`.
 */
template <typename Key, typename Value, typename KeyHash, typename KeyPred>
class Cache {
  public:
    /** Ctor
     *
     * The maximum number of elements may be zero (default), in which
     * case the cache is permitted to grow without limit.
     *
     * @exceptsafe Strong exception safety: exceptions will return the
     * system to previous state.
     */
    Cache(std::size_t maxElements=0) : _maxElements(maxElements) {
        _container.template get<Hash>().reserve(maxElements);
#ifdef LSST_CACHE_DEBUG
        _debuggingEnabled = false;
        _hits = 0;
        _total = 0;
        _requests.reserve(maxElements);
#endif
    }

    // Defaulted stuff
    Cache(Cache const &) = default;
    Cache(Cache &&) = default;
    Cache & operator=(Cache const &) = default;
    Cache & operator=(Cache &&) = default;

    /// Dtor
#ifdef LSST_CACHE_DEBUG
    ///
    /// Saves the cache requests to file.
    ~Cache();
#else
    ~Cache() = default;
#endif

    /** Lookup or generate a value
     *
     * If the key is in the cache, the corresponding value is returned.
     * Otherwise, a value is generated by the provided function which is
     * cached and returned. Thus, the (expensive) `Generator` function only
     * fires if the corresponding value is not already cached.
     *
     * The `Generator` function signature should be:
     *
     *     Value func(Key const& key);
     *
     * Given the possibility of lambdas, we could have made the required
     * function signature so that it took no arguments. However, it's
     * possible the user has some function that produces a value
     * when given a key, so chose to adopt that signature; any other
     * signature would likely require use of a lambda always.
     *
     * @exceptsafe Basic exception safety: exceptions will leave the
     * system in a valid but unpredictable state.
     */
    template <typename Generator>
    Value operator()(Key const& key, Generator func);

    /* Lookup a value
     *
     * If the key is in the cache, it will be promoted to the
     * most recently used value.
     *
     * @throws lsst::pex::exceptions::NotFoundError  If key is not in the
     * cache.
     *
     * @exceptsafe Basic exception safety: exceptions will leave the
     * system in a valid but unpredictable state.
     */
    Value operator[](Key const& key);

    /** Add a value to the cache
     *
     * If the key is already in the cache, the existing value will be
     * promoted to the most recently used value.
     *
     * @exceptsafe Basic exception safety: exceptions will leave the
     * system in a valid but unpredictable state.
     */
    void add(Key const& key, Value const& value);

    /** Return the number of values in the cache
     *
     * @exceptsafe Strong exception safety: exceptions will return the
     * system to previous state.
     */
    std::size_t size() const { return _container.size(); }

    /** Return all keys in the cache, most recent first
     *
     * @exceptsafe Strong exception safety: exceptions will return the
     * system to previous state.
     */
    std::vector<Key> keys() const;

    /** Does the cache contain the key?
     *
     * If the key is in the cache, it will be promoted to the
     * most recently used value.
     *
     * @exceptsafe Basic exception safety: exceptions will leave the
     * system in a valid but unpredictable state.
     */
    bool contains(Key const& key) { return _lookup(key).second; }

    /** Return the cached value if it exists.
     *
     * If the key is in the cache, it will be promoted to the
     * most recently used value.
     *
     * @exceptsafe Basic exception safety: exceptions will leave the
     * system in a valid but unpredictable state.
     */
    std::optional<Value> get(Key const& key) {
        auto result = _lookup(key);
        if (result.second) {
            return std::optional<Value>(result.first->second);
        } else {
            return std::optional<Value>();
        }
    }

    /** Return the capacity of the cache
     *
     * @exceptsafe No exceptions can be thrown.
     */
    std::size_t capacity() const { return _maxElements; }

    /** Change the capacity of the cache
     *
     * @exceptsafe Basic exception safety: exceptions will leave the
     * system in a valid but unpredictable state.
     */
    void reserve(std::size_t maxElements) { _maxElements = maxElements; _trim(); }

    /** Empty the cache
     *
     * @exceptsafe Basic exception safety: exceptions will leave the
     * system in a valid but unpredictable state.
     */
    void flush();

#ifdef LSST_CACHE_DEBUG
    void enableDebugging() { _debuggingEnabled = true; }
#endif

  private:

    // Trim the cache to size
    void _trim() {
        if (capacity() == 0) return;  // Allowed to grow without limit
        while (size() > capacity()) {
            _container.template get<Sequence>().pop_back();
        }
    }

    // Element in the multi_index container
    typedef std::pair<Key, Value> Element;

    // Tags for multi_index container
    struct Sequence {};
    struct Hash {};

    // The multi_index container
    typedef boost::multi_index_container<
                Element,
                boost::multi_index::indexed_by<
                    boost::multi_index::sequenced<boost::multi_index::tag<Sequence>>,
                    boost::multi_index::hashed_unique<
                        boost::multi_index::tag<Hash>,
                        boost::multi_index::member<Element, Key, &Element::first>,
                            KeyHash>>> Container;

    // Lookup key in the container
    //
    // Returns the iterator and whether there's anything there.
    //
    // If the key exists, updates the cache to make that key the most recent.
    std::pair<typename Container::template index<Hash>::type::iterator, bool> _lookup(Key const& key) {
        auto const& hashContainer = _container.template get<Hash>();
        auto it = hashContainer.find(key);
        bool found = (it != hashContainer.end());
        if (found) {
            _container.relocate(_container.template get<Sequence>().begin(),
                                _container.template project<Sequence>(it));
        }
#ifdef LSST_CACHE_DEBUG
        if (_debuggingEnabled) {
            _requests.push_back(key);
            ++_total;
            if (found) ++_hits;
        }
#endif
        return std::make_pair(it, found);
    }

    // Add a key-value pair that are not already present
    void _addNew(Key const& key, Value const& value) {
        _container.template get<Sequence>().emplace_front(key, value);
        _trim();
    }

    std::size_t _maxElements;  // Maximum number of elements; 0 means infinite
    Container _container;  // Container of key,value pairs
#ifdef LSST_CACHE_DEBUG
    bool _debuggingEnabled;
    mutable std::size_t _hits, _total;  // Statistics of cache hits
    mutable std::vector<Key> _requests;  // Cache requests
    static std::size_t getId() {
        static std::size_t _id = 0;  // Identifier
        return _id++;
    }
#endif
};

// Definitions

template <typename Key, typename Value, typename KeyHash, typename KeyPred>
template <typename Generator>
Value Cache<Key, Value, KeyHash, KeyPred>::operator()(
    Key const& key,
    Generator func
) {
    auto result = _lookup(key);
    if (result.second) {
        return result.first->second;
    }
    Value value = func(key);
    _addNew(key, value);
    return value;
}

template <typename Key, typename Value, typename KeyHash, typename KeyPred>
Value Cache<Key, Value, KeyHash, KeyPred>::operator[](Key const& key) {
    auto result = _lookup(key);
    if (result.second) {
        return result.first->second;
    }
    throw LSST_EXCEPT(pex::exceptions::NotFoundError,
                      (boost::format("Unable to find key: %s") % key).str());
}

template <typename Key, typename Value, typename KeyHash, typename KeyPred>
void Cache<Key, Value, KeyHash, KeyPred>::add(Key const& key, Value const& value) {
    auto result = _lookup(key);
    if (!result.second) {
        _addNew(key, value);
    }
}

template <typename Key, typename Value, typename KeyHash, typename KeyPred>
std::vector<Key> Cache<Key, Value, KeyHash, KeyPred>::keys() const {
    std::vector<Key> result;
    result.reserve(size());
    for (auto & keyValue : _container.template get<Sequence>()) {
        result.push_back(keyValue.first);
    }
    return result;
}

template <typename Key, typename Value, typename KeyHash, typename KeyPred>
void Cache<Key, Value, KeyHash, KeyPred>::flush() {
    while (size() > 0) {
        _container.template get<Sequence>().pop_back();
    }
}

#ifdef LSST_CACHE_DEBUG
template <typename Key, typename Value, typename KeyHash, typename KeyPred>
Cache<Key, Value, KeyHash, KeyPred>::~Cache() {
    if (!_debuggingEnabled) {
        return;
    }
    std::string filename = (boost::format("lsst-cache-%s-%d.dat") %
                            demangleType(typeid(*this).name()) % getId()).str();
    std::ofstream file(filename);
    for (auto const& key : _requests) {
        file << key << "\n";
    }
    file.close();
    std::cerr << "Wrote cache requests to " << filename << ": " << _hits << "/" << _total << " hits";
    std::cerr << std::endl;
}
#endif

}
} // namespace lsst::cpputils


#endif // ifndef LSST_CPPUTILS_CACHE_H
