// -*- lsst-c++ -*-
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

#ifndef LSST_CPPUTILS_PYTHON_TEMPLATEINVOKER_H
#define LSST_CPPUTILS_PYTHON_TEMPLATEINVOKER_H

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/ndarray.h>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <map>

namespace lsst { namespace cpputils { namespace python {

/**
 * A helper class for wrapping C++ template functions as Python functions with dtype arguments.
 *
 * TemplateInvoker takes a templated callable object, a `nanobind::dlpack::dtype`
 * object, and a sequence of supported C++ types via its nested `Tag` struct.
 * The callable is invoked with a scalar argument of the type matching the
 * `dtype` object.  If none of the supported C++ types match, a different
 * error callback is invoked instead.
 *
 * As an example, we'll wrap this function:
 * @code
 * template <typename T>
 * T doSomething(std::string const & argument);
 * @endcode
 *
 * TemplateInvoker provides a default error callback, which we'll use here
 * (otherwise you'd need to pass one when constructing the TemplateInvoker).
 *
 * For the main callback, we'll define this helper struct:
 * @code
 * struct DoSomethingHelper {
 *
 *     template <typename T>
 *     T operator()(T) const {
 *         return doSomething<T>(argument);
 *     }
 *
 *     std::string argument;
 * };
 * @endcode
 *
 * The pybind11 wrapper for `doSomething` is then another lambda that uses
 * `TemplateInvoker::apply` to call the helper:
 * @code
 * mod.def(
 *     "doSomething",
 *     [](std::string const & argument, py::dtype const & dtype) {
 *         return TemplateInvoker().apply(
 *             DoSomethingHelper{argument},
 *             dtype,
 *             TemplateInvoker::Tag<int, float, double>()
 *         );
 *     },
 *     "argument"_a
 * );
 * @endcode
 *
 * The type returned by the helper callable's `operator()` can be anything
 * pybind11 knows how to convert to Python.
 *
 * While defining a full struct with a templated `operator()` makes it more
 * obvious what TemplateInvoker is doing, it's much more concise to use a
 * universal lambda with the `decltype` operator. This wrapper is equivalent
 * to the one above, but it doesn't need `DoSomethingHelper`:
 * @code
 * mod.def(
 *     "doSomething",
 *     [](std::string const & argument, py::dtype const & dtype) {
 *         return TemplateInvoker().apply(
 *             [&argument](auto t) { return doSomething<decltype(t)>(argument); },
 *             dtype,
 *             TemplateInvoker::Tag<int, float, double>()
 *         );
 *     },
 *     "argument"_a
 * );
 * @endcode
 * Note that the value of `t` here doesn't matter; what's important is that its
 * C++ type corresponds to the type passed in the `dtype` argument.  So instead
 * of using that value, we use the `decltype` operator to extract that type and
 * use it as a template parameter.
 */
class TemplateInvoker {
public:

    /// A simple tag type used to pass one or more types as a function argument.
    template <typename ...Types>
    struct Tag {};

    /// Callback type for handling unmatched-type errors.
    using OnErrorCallback = std::function<nanobind::object(nanobind::object const & dtype)>;

    /// Callback used for handling unmatched-type errors by default.
    static nanobind::object handleErrorDefault(nanobind::object const & dtype) {
        PyErr_Format(PyExc_TypeError, "dtype '%s' not supported.", dtype.ptr()->ob_type->tp_name);
        throw nanobind::type_error();
    }

    /**
     * Construct a TemplateInvoker that calls the given object when no match is found.
     *
     * The callback should have the same signature as handleErrorDefault; the
     * dtype actually passed from Python is passed so it can be included in
     * error messages.
     */
    explicit TemplateInvoker(OnErrorCallback onError) : _onError(std::move(onError)) {}

    /// Construct a TemplateInvoker that calls handleErrorDefault when no match is found.
    TemplateInvoker() : TemplateInvoker(handleErrorDefault) {}

    /**
     * Call and return `function(static_cast<T>(0))` with the type T that
     * matches a given NumPy `dtype` object.
     *
     * @param[in]  function     Callable object to invoke.  Must have an
     *                          overloaded operator() that takes any `T` in
     *                          the sequence `TypesToTry`, and a
     *                          `fail(py::dtype)` method to handle the case
    *                          where none of the given types match.
     *
     * @param[in]  dtype        NumPy dtype object indicating the template
     *                          specialization to invoke.
     *
     * @param[in]  typesToTry   A `Tag` instance parameterized with the list of
     *                          types to try to match to `dtype`.
     *
     * @return the result of calling `function` with the matching type, after
     *         converting it into a Python object.
     *
     * @exceptsafe the same as the exception safety of `function`
     */
    template <typename Function, typename ...TypesToTry>
    nanobind::object apply(
        Function function,
        nanobind::object const & dtype,
        Tag<TypesToTry...> typesToTry
    ) const {
        return _apply(function, dtype, typesToTry);
    }

private:

    template <typename Function>
    nanobind::object _apply(Function & function, nanobind::object const & dtype, Tag<>) const {
        return _onError(dtype);
    }
    static nb::dlpack::dtype get_dtype(std::string const &type) {
        static const std::map<std::string, nb::dlpack::dtype> dtype_map = {
                {"uint8", nb::dtype<uint8_t>()},
                {"uint16", nb::dtype<uint16_t>()},
                {"uint32", nb::dtype<uint32_t>()},
                {"uint64", nb::dtype<uint64_t>()},
                {"int8", nb::dtype<int8_t>()},
                {"int16", nb::dtype<int16_t>()},
                {"int32", nb::dtype<int32_t>()},
                {"int64", nb::dtype<int64_t>()},
                {"float32", nb::dtype<float>()},
                {"float64", nb::dtype<double>()}
        };
        try {
            auto result = dtype_map.at(type);
            return result;
        } catch(...) {
            throw std::out_of_range("TemplateInvoker: Invalid type " + type);
        }

    }

    template <typename Function, typename T, typename ...A>
    nanobind::object _apply(Function & function, nanobind::object const & dtype, Tag<T, A...>) const {
        auto name = nb::cast<std::string>(dtype.attr("name"));
        auto type = get_dtype(name);
        if(type == nb::dtype<T>()) return nb::cast(function(static_cast<T>(0)));
        return _apply(function, dtype, Tag<A...>());
    }

    OnErrorCallback _onError;
};

}}
}  // namespace lsst::cpputils::python

#endif
