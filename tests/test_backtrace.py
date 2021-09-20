#
# Developed for the LSST Data Management System.
# This product includes software developed by the LSST Project
# (https://www.lsst.org).
# See the COPYRIGHT file at the top-level directory of this distribution
# for details of code ownership.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

import os
import sys
import unittest
import subprocess

from lsst.cpputils import backtrace

ROOT = os.path.abspath(os.path.dirname(__file__))


class BacktraceTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def test_segfault(self):
        if backtrace.isEnabled():
            with self.assertRaises(subprocess.CalledProcessError) as cm:
                subprocess.check_output([sys.executable, os.path.join(ROOT, "backtrace.py")],
                                        stderr=subprocess.STDOUT)

            output = cm.exception.output.decode()
            print(output)
            self.assertIn("backtrace follows", output)


if __name__ == "__main__":
    unittest.main()
