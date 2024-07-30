/*
 * Developed for the LSST Data Management System.
 * This product includes software developed by the LSST Project
 * (https://www.lsst.org).
 * See the COPYRIGHT file at the top-level directory of this distribution
 * for details of code ownership.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "lsst/cpputils/packaging.h"

#define BOOST_TEST_MODULE packaging
#define BOOST_TEST_DYN_LINK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "boost/test/unit_test.hpp"
#pragma clang diagnostic pop

#include <filesystem>
#include "lsst/pex/exceptions.h"

using namespace lsst::cpputils;

BOOST_AUTO_TEST_SUITE(PackagingSuite)

BOOST_AUTO_TEST_CASE(GetPackage) {
    std::filesystem::path cpputilsPath{getPackageDir("cpputils")};
    BOOST_CHECK(std::filesystem::is_regular_file(cpputilsPath / "tests" / "test_packaging.cc"));
    BOOST_CHECK_THROW(getPackageDir("nameOfNonexistendPackage2234q?#!"),
                      lsst::pex::exceptions::NotFoundError);
}

BOOST_AUTO_TEST_SUITE_END()
