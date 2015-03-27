// -*- lsst-c++ -*-

/* 
 * LSST Data Management System
 * Copyright 2008, 2009, 2010 LSST Corporation.
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
 
%define testLib_DOCSTRING
"
Test module for various utilities in p_lsstSwig.i
"
%enddef

%feature("autodoc", "1");
%module(package="testLib", docstring=testLib_DOCSTRING) testLib

%pythonnondynamic;
%naturalvar;  // use const reference typemaps

%{
#include <boost/cstdint.hpp>
%}

%include "lsst/p_lsstSwig.i"

%lsst_exceptions()

%returnNone(Example::get1)
%returnSelf(Example::get2)
%returnCopy(Example::get3)
%addStreamRepr(Example)
%useValueEquality(Example)

%inline %{
    class Example {
    public:

        explicit Example(std::string const & v) : _value(v) {}

        explicit Example(bool b) : _value("boolean") {}

        Example(Example const & other) : _value(other._value) {}

        Example & get1() { return *this; }
        Example & get2() { return *this; }
        Example & get3() { return *this; }

        std::string getValue() const { return _value; }
        void setValue(std::string const & v) { _value = v; }
        
        bool operator==(Example const & other) const {
            return other._value == _value;
        }
        bool operator!=(Example const & other) const {
            return !(*this == other);
        }

    private:
        std::string _value;
    };

#ifndef SWIG
    inline std::ostream & operator<<(std::ostream & os, Example const & ex) {
        return os << "Example(" << ex.getValue() << ")";
    }
#endif

    template <typename T>
    bool acceptNumber(T value) { return true; }

%}

%template(accept_int8) acceptNumber<boost::int8_t>;
%template(accept_uint8) acceptNumber<boost::uint8_t>;
%template(accept_int16) acceptNumber<boost::int16_t>;
%template(accept_uint16) acceptNumber<boost::uint16_t>;
%template(accept_int32) acceptNumber<boost::int32_t>;
%template(accept_uint32) acceptNumber<boost::uint32_t>;
%template(accept_int64) acceptNumber<boost::int64_t>;
%template(accept_uint64) acceptNumber<boost::uint64_t>;
%template(accept_float32) acceptNumber<float>;
%template(accept_float64) acceptNumber<double>;
