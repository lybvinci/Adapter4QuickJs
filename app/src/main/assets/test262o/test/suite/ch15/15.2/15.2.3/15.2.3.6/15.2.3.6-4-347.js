/// Copyright (c) 2012 Ecma International.  All rights reserved. 
/// Ecma International makes this code available under the terms and conditions set
/// forth on http://hg.ecmascript.org/tests/test262/raw-file/tip/LICENSE (the 
/// "Use Terms").   Any redistribution of this code must retain the above 
/// copyright and this notice and otherwise comply with the Use Terms.
/**
 * @path ch15/15.2/15.2.3/15.2.3.6/15.2.3.6-4-347.js
 * @description ES5 Attributes - property ([[Writable]] is true, [[Enumerable]] is false, [[Configurable]] is false) is writable
 */


function testcase() {
        var obj = {};

        Object.defineProperty(obj, "prop", {
            value: 2010,
            writable: true,
            enumerable: false,
            configurable: false
        });
        var propertyDefineCorrect = (obj.prop === 2010);
        obj.prop = 1001;

        return propertyDefineCorrect && obj.prop === 1001;
    }
runTestCase(testcase);
