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

import unittest

from _cache import NumbersCache


def numberToWords(value):
    """Convert a number in the range [0, 1000) to words"""
    assert value >= 0 and isinstance(value, int), "Only non-negative integers"
    if value < 20:
        return {
            0: "zero",
            1: "one",
            2: "two",
            3: "three",
            4: "four",
            5: "five",
            6: "six",
            7: "seven",
            8: "eight",
            9: "nine",
            10: "ten",
            11: "eleven",
            12: "twelve",
            13: "thirteen",
            14: "fourteen",
            15: "fifteen",
            16: "sixteen",
            17: "seventeen",
            18: "eighteen",
            19: "nineteen",
        }[value]
    if value < 100:
        tens = value//10
        ones = value % 10
        return {
            2: "twenty",
            3: "thirty",
            4: "forty",
            5: "fifty",
            6: "sixty",
            7: "seventy",
            8: "eighty",
            9: "ninety",
        }[tens] + (("-" + numberToWords(ones)) if ones > 0 else "")
    assert value < 1000, "Value exceeds limit of 999"
    hundreds = value//100
    rest = value % 100
    return numberToWords(hundreds) + " hundred" + ((" " + numberToWords(rest)) if rest > 0 else "")


class CacheTestCase(unittest.TestCase):
    """Tests of lsst.cpputils.Cache"""
    def check(self, addFunction):
        """Exercise the Cache

        The `addFunction` should take a cache and number,
        and add the number (and its corresponding string)
        into the cache.
        """
        capacity = 10
        cache = NumbersCache(capacity)
        self.assertEqual(cache.size(), 0, "Starts empty")
        self.assertEqual(cache.capacity(), capacity, "Capacity as requested")
        maximum = 20
        for ii in range(maximum):
            addFunction(cache, ii)
        self.assertEqual(cache.size(), capacity, "Filled to capacity")
        self.assertEqual(cache.capacity(), capacity, "Capacity unchanged")
        for ii in range(maximum - capacity):
            self.assertNotIn(ii, cache, "Should have been expunged")
            self.assertIsNone(cache.get(ii), "Should have been expunged")
        expectedContents = list(range(maximum - 1, maximum - capacity - 1, -1))  # Last in, first out
        actualContents = cache.keys()
        for ii in expectedContents:
            self.assertIn(ii, cache, "Should be present")
            self.assertEqual(cache[ii], numberToWords(ii), "Value accessible and as expected")
            self.assertEqual(cache.get(ii), numberToWords(ii), "Value accessible via get and as expected")
        self.assertListEqual(actualContents, expectedContents, "Contents are as expected")
        with self.assertRaises(LookupError):
            cache[maximum - capacity - 1]
        newCapacity = 5
        cache.reserve(newCapacity)
        # The new list of contents is smaller, but also reversed because we've gone through the cache
        # touching items.
        newExpectedContents = list(reversed(expectedContents))[:newCapacity]
        self.assertEqual(cache.capacity(), newCapacity, "Capacity changed")
        self.assertEqual(cache.size(), newCapacity, "Size changed to correspond to new capacity")
        self.assertListEqual(cache.keys(), newExpectedContents, "Most recent kept")
        cache.flush()
        self.assertEqual(cache.size(), 0, "Flushed everything out")
        self.assertEqual(cache.capacity(), newCapacity, "Capacity unchanged")
        return cache

    def testDirect(self):
        """Directly add key,value pairs into the cache with Cache.add"""
        self.check(lambda cache, index: cache.add(index, numberToWords(index)))

    def testLazy(self):
        """Exercise the lazy function call in Cache.operator()"""
        def addFunction(cache, index):
            value = cache(index, lambda ii: numberToWords(ii))
            self.assertEqual(value, numberToWords(index))

        cache = self.check(addFunction)

        # Check that the function call doesn't fire when we pull out something that's in there already
        def trap(key):
            raise AssertionError("Failed")

        for index in cache.keys():
            value = cache(index, trap)
            self.assertEqual(value, numberToWords(index))

        # Check that this checking technique actually works...
        with self.assertRaises(AssertionError):
            cache(999, trap)


if __name__ == "__main__":
    unittest.main()
