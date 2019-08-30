// Copyright 2019 Mathias Bynens. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
author: Mathias Bynens
description: >
  Unicode property escapes for `Script=Miao`
info: |
  Generated by https://github.com/mathiasbynens/unicode-property-escapes-tests
  Unicode v12.1.0
esid: sec-static-semantics-unicodematchproperty-p
features: [regexp-unicode-property-escapes]
includes: [regExpUtils.js]
---*/

const matchSymbols = buildString({
  loneCodePoints: [],
  ranges: [
    [0x016F00, 0x016F4A],
    [0x016F4F, 0x016F87],
    [0x016F8F, 0x016F9F]
  ]
});
testPropertyEscapes(
  /^\p{Script=Miao}+$/u,
  matchSymbols,
  "\\p{Script=Miao}"
);
testPropertyEscapes(
  /^\p{Script=Plrd}+$/u,
  matchSymbols,
  "\\p{Script=Plrd}"
);
testPropertyEscapes(
  /^\p{sc=Miao}+$/u,
  matchSymbols,
  "\\p{sc=Miao}"
);
testPropertyEscapes(
  /^\p{sc=Plrd}+$/u,
  matchSymbols,
  "\\p{sc=Plrd}"
);

const nonMatchSymbols = buildString({
  loneCodePoints: [],
  ranges: [
    [0x00DC00, 0x00DFFF],
    [0x000000, 0x00DBFF],
    [0x00E000, 0x016EFF],
    [0x016F4B, 0x016F4E],
    [0x016F88, 0x016F8E],
    [0x016FA0, 0x10FFFF]
  ]
});
testPropertyEscapes(
  /^\P{Script=Miao}+$/u,
  nonMatchSymbols,
  "\\P{Script=Miao}"
);
testPropertyEscapes(
  /^\P{Script=Plrd}+$/u,
  nonMatchSymbols,
  "\\P{Script=Plrd}"
);
testPropertyEscapes(
  /^\P{sc=Miao}+$/u,
  nonMatchSymbols,
  "\\P{sc=Miao}"
);
testPropertyEscapes(
  /^\P{sc=Plrd}+$/u,
  nonMatchSymbols,
  "\\P{sc=Plrd}"
);
