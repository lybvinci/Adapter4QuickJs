// Copyright (C) 2017 Mathias Bynens.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.
/*---
description: |
    Collection of functions used to assert the correctness of RegExp objects.
---*/

if ($262 && typeof $262.codePointRange === "function") {
    /* use C function to build the codePointRange (much faster with
       slow JS engines) */
    codePointRange = $262.codePointRange;
} else {
    codePointRange = function codePointRange(start, end) {
        const codePoints = [];
        let length = 0;
        for (codePoint = start; codePoint < end; codePoint++) {
            codePoints[length++] = codePoint;
        }
        return String.fromCodePoint.apply(null, codePoints);
    }
}

function buildString({ loneCodePoints, ranges }) {
    let result = String.fromCodePoint.apply(null, loneCodePoints);
    for (const [start, end] of ranges) {
        result += codePointRange(start, end + 1);
    }
    return result;
}

function testPropertyEscapes(regex, string, expression) {
  if (!regex.test(string)) {
    for (const symbol of string) {
      const hex = symbol
        .codePointAt(0)
        .toString(16)
        .toUpperCase()
        .padStart(6, "0");
      assert(
        regex.test(symbol),
        `\`${ expression }\` should match U+${ hex } (\`${ symbol }\`)`
      );
    }
  }
}

// Returns a function that will validate RegExp match result
//
// Example:
//
//    var validate = matchValidator(['b'], 1, 'abc');
//    validate(/b/.exec('abc'));
//
function matchValidator(expectedEntries, expectedIndex, expectedInput) {
  return function(match) {
    assert.compareArray(match, expectedEntries, 'Match entries');
    assert.sameValue(match.index, expectedIndex, 'Match index');
    assert.sameValue(match.input, expectedInput, 'Match input');
  }
}
