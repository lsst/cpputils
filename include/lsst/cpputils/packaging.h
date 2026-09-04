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

#ifndef LSST_CPPUTILS_PACKAGING_H
#define LSST_CPPUTILS_PACKAGING_H

#include <string>

namespace lsst {
namespace cpputils {

/*!
 * \brief return the root directory of a setup package
 *
 * \param[in] packageName  name of package (e.g. "utils")
 *
 * \throw lsst::pex::exceptions::NotFoundError if desired version can't be found
 */
std::string getPackageDir(std::string const& packageName);

/*!
 * \brief return the root directory of the package whose shared library contains
 *        the given address
 *
 * The address is resolved to its containing shared library via `dladdr`, and the
 * package root is derived from that library's location (`.../lib/libFoo.so` ->
 * `...`).  This allows a package to locate its own data files without relying on
 * environment variables, provided the address belongs to a symbol compiled into
 * that package's own shared library.
 *
 * \param[in] addressInLibrary  address of a symbol residing in the target
 *                              package's shared library
 *
 * \throw lsst::pex::exceptions::NotFoundError if the library cannot be located
 */
std::string getPackageDirFromAddress(void const* addressInLibrary);

}
} // namespace lsst::cpputils

#endif
