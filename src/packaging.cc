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

#include "lsst/cpputils/packaging.h"

#include <dlfcn.h>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include "lsst/pex/exceptions.h"

namespace lsst {
namespace cpputils {

std::string getPackageDir(std::string const& packageName) {
    std::string envVar = packageName;      // package's environment variable

    transform(envVar.begin(), envVar.end(), envVar.begin(), (int (*)(int)) toupper);
    envVar += "_DIR";

    char const *dir = getenv(envVar.c_str());
    if (!dir) {
        throw LSST_EXCEPT(lsst::pex::exceptions::NotFoundError, "Package " + packageName + " not found");
    }

    return dir;
}

std::string getPackageDirFromAddress(void const* addressInLibrary) {
    // dladdr resolves an address to the shared object whose memory map contains
    // it, regardless of which library's code is calling dladdr.  Passing an
    // address from the target package's own library therefore yields that
    // library's path (works identically for .so on Linux and .dylib on macOS).
    Dl_info info{};
    if (dladdr(addressInLibrary, &info) == 0 || info.dli_fname == nullptr) {
        throw LSST_EXCEPT(lsst::pex::exceptions::NotFoundError,
                          "Could not locate the shared library for the given address");
    }

    std::error_code ec;
    std::filesystem::path libraryPath = std::filesystem::canonical(info.dli_fname, ec);
    if (ec) {
        throw LSST_EXCEPT(lsst::pex::exceptions::NotFoundError,
                          "Could not resolve shared library path '" +
                                  std::string(info.dli_fname) + "': " + ec.message());
    }

    // .../<packageRoot>/lib/lib<name>.{so,dylib} -> <packageRoot>
    return libraryPath.parent_path().parent_path().string();
}

}} // namespace lsst::cpputils
