package com.lybvinci.adapter4quickjs;

import android.content.Context;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import java.io.File;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class TestES6 {
    private static final String root = "test262";
    private static final String TAG = "TestES6";

    private static List<String> excludeTestList = Arrays.asList("test262/test/intl402/", "test262/test/built-ins/BigInt",
            "test262/test/built-ins/Function/prototype/restricted-property-caller.js",
            "test262/test/built-ins/ThrowTypeError/unique-per-realm-function-proto.js",
            "test262/test/language/statements/debugger/statement.js",
            "test262/test/language/expressions/dynamic-import/for-await-resolution-and-error-agen.js",
            "test262/test/language/expressions/dynamic-import/for-await-resolution-and-error.js");

    private static List<String> supportFeature = Arrays.asList("Array.prototype.flat",
            "Array.prototype.flatMap",
            "Array.prototype.flatten",
            "Array.prototype.values",
            "ArrayBuffer",
            "arrow-function",
            "async-functions",
            "async-iteration",
            "Atomics",
//            "BigInt=skip",
            "caller",
            "class",
            "class-fields-private",
            "class-fields-public",
            "class-methods-private",
            "class-static-fields-public",
            "class-static-fields-private",
            "class-static-methods-private",
            "computed-property-names",
            "const",
//            "cross-realm=skip",
            "DataView",
            "DataView.prototype.getFloat32",
            "DataView.prototype.getFloat64",
            "DataView.prototype.getInt16",
            "DataView.prototype.getInt32",
            "DataView.prototype.getInt8",
            "DataView.prototype.getUint16",
            "DataView.prototype.getUint32",
            "DataView.prototype.setUint8",
            "default-arg",
            "default-parameters",
            "destructuring-assignment",
            "destructuring-binding",
            "dynamic-import",
            "export-star-as-namespace-from-module",
//            "FinalizationGroup=skip",
            "Float32Array",
            "Float64Array",
            "for-of",
            "generators",
            "globalThis",
            "hashbang",
//            "host-gc-required=skip",
            "import.meta",
            "Int32Array",
            "Int8Array",
//            "IsHTMLDDA=skip",
            "json-superset",
            "let",
            "Map",
            "new.target",
            "numeric-separator-literal",
            "object-rest",
            "object-spread",
            "Object.fromEntries",
            "Object.is",
            "optional-catch-binding",
//            "optional-chaining=skip",
            "Promise.allSettled",
            "Promise.prototype.finally",
            "Proxy",
            "proxy-missing-checks",
            "Reflect",
            "Reflect.construct",
            "Reflect.set",
            "Reflect.setPrototypeOf",
            "regexp-dotall",
            "regexp-lookbehind",
            "regexp-named-groups",
            "regexp-unicode-property-escapes",
            "rest-parameters",
            "Set",
            "SharedArrayBuffer",
            "string-trimming",
            "String.fromCodePoint",
            "String.prototype.endsWith",
            "String.prototype.includes",
            "String.prototype.matchAll",
            "String.prototype.trimEnd",
            "String.prototype.trimStart",
            "super",
            "Symbol",
            "Symbol.asyncIterator",
            "Symbol.hasInstance",
            "Symbol.isConcatSpreadable",
            "Symbol.iterator",
            "Symbol.match",
            "Symbol.matchAll",
            "Symbol.prototype.description",
            "Symbol.replace",
            "Symbol.search",
            "Symbol.species",
            "Symbol.split",
            "Symbol.toPrimitive",
            "Symbol.toStringTag",
            "Symbol.unscopables",
//            "tail-call-optimization=skip",
            "template",
//            "top-level-await=skip",
            "TypedArray",
            "u180e",
            "Uint16Array",
            "Uint8Array",
            "Uint8ClampedArray",
            "WeakMap",
//            "WeakRef=skip",
            "WeakSet",
            "well-formed-json-stringify");

    private static final String REGEXP = "features: \\[(.*?)\\]";;

    private Pattern mPattern;

    private LinkedList<String> failedPath;
    private LinkedList<String> skipList;
    private List<String> depends = Arrays.asList("sta.js", "assert.js", "doneprintHandle.js");
    private String baseJS = "";
    private JSDelegate mDelegate;

    public TestES6(JSDelegate delegate) {
        this.mDelegate = delegate;
        mPattern = Pattern.compile(REGEXP);
        failedPath = new LinkedList<>();
        skipList = new LinkedList<>();
    }

    public void runTest() {
        mDelegate.startLoading();

        File filesDir = mDelegate.getContext().getFilesDir();
        File test262_1 = new File(filesDir, "test262");
        File test262 = new File(test262_1, "test262");

        makeBaseJS(new File(test262, "harness"));

//        executeDir(test262, "harness", false);

        File test = new File(test262, "test");
        // has success!
//        executeDir(test, "harness");
        executeDir(test, "built-ins");

        //exclude
//        executeDir(test, "intl402");

        executeDir(test, "annexB");
//        executeDir(test, "language");

        Log.i(TAG, "------------------- js test finished result -------------------------");
        Log.i(TAG, "test failed count=" + failedPath.size());
        Log.i(TAG, "[");
        for (String path : failedPath) {
            Log.i(TAG, path + ",");
        }
        Log.i(TAG, "]");
        Log.i(TAG, "test skip count =" + skipList.size());
        Log.i(TAG, "[");
        for (String path : skipList) {
            Log.i(TAG, path + ",");
        }
        Log.i(TAG, "------------------- js test finished result end -------------------------");
        Toast.makeText(mDelegate.getContext(), "测试完成", Toast.LENGTH_LONG).show();

        mDelegate.stopLoading();

    }

    private void executeDir(File parent, String child) {
        executeDir(parent, child, true);
    }

    private void executeDir(File parent, String child, boolean useBase) {
        File dir = new File(parent, child);
        LinkedList<String> sourcePath = new LinkedList<>();
        loadFiles(dir, sourcePath);
        for (String path : sourcePath) {
            executeJs(new File(path), useBase);
        }

    }

    private void makeBaseJS(File file) {
        LinkedList<String> sourcePath = new LinkedList<>();
        loadFiles(file, sourcePath);
//        List<String> dependPath = new LinkedList<>();
//        for (String path : sourcePath) {
//            for (String depend : depends) {
//                if (path.endsWith(depend)) {
//                    dependPath.add(path);
//                }
//            }
//        }
        for (String depend : sourcePath) {
            File f = new File(depend);
            baseJS += Utils.readJSFile(f) + "\n";
        }
        Log.i(TAG, "baseJS is" + baseJS);
    }

    private void loadFiles(File root, LinkedList<String> paths) {
        if (!root.exists()) {
            return;
        }
        if (root.isDirectory()) {
            File[] files = root.listFiles();
            for (File f : files) {
                loadFiles(f, paths);
            }
        } else {
            if (root.getName().endsWith(".js")) {
                for (String exclude : excludeTestList) {
                    if (root.getAbsolutePath().contains(exclude)){
                        skipList.add(root.getPath());
                        return;
                    }
                }
                paths.add(root.getAbsolutePath());
            }
        }
    }

    public void executeJs(File dest, boolean useBase) {
        String data = Utils.readJSFile(dest);
        if (!isSupport(data, dest.getAbsolutePath())) {
            Log.w(TAG, "skip test=" + root);
            return;
        }
        if (useBase) {
            data = baseJS + data;
        }
        if (null != data) {
            int ret = mDelegate.executeJS(data, dest.getPath());
            if (ret != 0) {
                failedPath.add(dest.getPath());
            }
        } else {
            Log.e(TAG, dest.getPath() + " bytes is null!!!");
        }
    }

    private boolean isSupport(String jsdata, String filename) {
        Matcher matcher = mPattern.matcher(jsdata);
        if (matcher.find()) {
            String group1 = matcher.group(0);
            int start = group1.indexOf("[");
            int end = group1.lastIndexOf("]");
            String[] features = group1.substring(start + 1, end).split(",");
            for (String feature: features) {
                if (!supportFeature.contains(feature)) {
                    skipList.add(filename);
                    return false;
                }
            }
        }
        return true;
    }

    public void unzipResource() {
        mDelegate.startLoading();
        final File filesDir = mDelegate.getContext().getFilesDir();
        final File test262 = new File(filesDir, "test262");
        if (!test262.exists()) {
            test262.mkdir();
        } else {
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Utils.unZipAssetsFolder(mDelegate.getContext(), "test262.zip", test262.getAbsolutePath());
                } catch (Exception e) {
                    e.printStackTrace();
                }
                mDelegate.stopLoading();
            }
        }).run();
    }


}
