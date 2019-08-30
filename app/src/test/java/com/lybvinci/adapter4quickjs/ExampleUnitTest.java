package com.lybvinci.adapter4quickjs;

import android.util.Log;

import org.junit.Test;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static org.junit.Assert.*;

/**
 * Example local unit test, which will execute on the development machine (host).
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
public class ExampleUnitTest {
    private static final String REGEXP = "features: \\[(.*?)\\]";
    String test = "// Copyright (C) 2016 the V8 project authors. All rights reserved.\n" +
            "// This code is governed by the BSD license found in the LICENSE file.\n" +
            "/*---\n" +
            "esid: sec-boolean-constructor-boolean-value\n" +
            "description: Default [[Prototype]] value derived from realm of the newTarget\n" +
            "info: |\n" +
            "    [...]\n" +
            "    3. Let O be ? OrdinaryCreateFromConstructor(NewTarget,\n" +
            "       \"%BooleanPrototype%\", « [[BooleanData]] »).\n" +
            "    [...]\n" +
            "\n" +
            "    9.1.14 GetPrototypeFromConstructor\n" +
            "\n" +
            "    [...]\n" +
            "    3. Let proto be ? Get(constructor, \"prototype\").\n" +
            "    4. If Type(proto) is not Object, then\n" +
            "       a. Let realm be ? GetFunctionRealm(constructor).\n" +
            "       b. Let proto be realm's intrinsic object named intrinsicDefaultProto.\n" +
            "    [...]\n" +
            "features: [cross-realm, Reflect]\n" +
            "---*/\n" +
            "\n" +
            "var other = $262.createRealm().global;\n" +
            "var C = new other.Function();\n" +
            "C.prototype = null;\n" +
            "\n" +
            "var o = Reflect.construct(Boolean, [], C);\n" +
            "\n" +
            "assert.sameValue(Object.getPrototypeOf(o), other.Boolean.prototype);\n";
    @Test
    public void addition_isCorrect() {
        supprt(test);
    }

    private boolean supprt(String jsdata) {
        Pattern mPattern = Pattern.compile(REGEXP);
        Matcher matcher = mPattern.matcher(jsdata);
        if (matcher.find()) {
            int count = matcher.groupCount();
            for (int i = 0; i < count; i++) {
                String group = matcher.group(i);
                System.out.println("group=" + group);
            }
        }
        return true;
    }
}