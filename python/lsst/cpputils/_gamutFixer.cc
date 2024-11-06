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
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "lsst/cpputils/python.h"
#include "lsst/cpputils/_oklabTools.h"

namespace py = pybind11;

namespace lsst {
namespace cpputils {


py::array_t<double> fixGamutOK(py::array_t<double, py::array::c_style | py::array::forcecast> & Lab_points) {
  py::buffer_info Lab_buffer = Lab_points.request();
  auto Lab_ptr = Lab_points.unchecked<2>();
  py::array_t<double> result(Lab_buffer.shape);
  py::buffer_info result_buffer = result.request();
  auto result_ptr = result.mutable_unchecked<2>();
  float alpha = 0.5f;

  for (int pixel_number=0; pixel_number < Lab_buffer.shape[0]; pixel_number++){
    double L = Lab_ptr(pixel_number, 0);
    double a = Lab_ptr(pixel_number, 1);
    double b = Lab_ptr(pixel_number, 2);
    double esp = 0.00001;
    float C = std::max(esp, sqrt(a*a + b*b));
    float a_ = a/C;
    float b_ = b/C;

    details::LC cusp = details::find_cusp(a_, b_);
  	float Ld = L - cusp.L;
  	float k = 2.f * (Ld > 0 ? 1.f - cusp.L : cusp.L);

  	float e1 = 0.5f*k + fabs(Ld) + alpha * C/k;
  	float L0 = cusp.L + 0.5f * (details::sgn(Ld) * (e1 - sqrtf(e1 * e1 - 2.f * k * fabs(Ld))));

  	float t = details::find_gamut_intersection(a_, b_, L, C, L0);
  	float L_clipped = L0 * (1.f - t) + t * L;
  	float C_clipped = t * C;

    result_ptr(pixel_number, 0) = L_clipped;
    result_ptr(pixel_number, 1) = C_clipped * a_;
    result_ptr(pixel_number, 2) = C_clipped * b_;
  }
  return result;

}

void wrapFixGamut(lsst::cpputils::python::WrapperCollection &wrappers) {
    wrappers.wrap([](auto &mod) {
        mod.def("fixGamutOK", &fixGamutOK,"");
    });
}

}
}

