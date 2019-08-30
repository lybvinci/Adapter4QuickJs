package com.lybvinci.adapter4quickjs;

import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public class TestES5 {

    private static final String TAG = "TestES5";
    private JSDelegate mDelegate;
    private LinkedList<String> mFailedPaths;

    private String baseJS = "";

    private static final List<String> DEPENDS = Arrays.asList("sth.js");
    private static final List<String> EXCLUDE_SOURCE = Arrays.asList("");

    public TestES5(JSDelegate jsDelegate) {
        mDelegate = jsDelegate;
        mFailedPaths = new LinkedList<>();

    }


    public void runTest() {
        mDelegate.startLoading();
        File fileDir = mDelegate.getContext().getFilesDir();
        File es5test = new File(fileDir, "es5test");
        String harness = makeBaseJS(new File(es5test, "harness"));
        baseJS = harness;
        executeDir(fileDir, "es5test");
//        File combineJs = new File(fileDir, "combine.js");
//        if (combineJs.exists()) {
//            String s = Utils.readJSFile(combineJs);
//            executeJs(s, "combine.js");
//        } else {
//            File testES5Dir = new File(fileDir, "es5test");
//            String baseJS = makeBaseJS(new File(testES5Dir, "harness"));
//            String testScript = makeJS(new File(testES5Dir, "TestCases"));
//            baseJS = baseJS + testScript + endScript;
//            writeToFile(baseJS);
//            executeJs(baseJS, "combine.js");
//        }
        mDelegate.stopLoading();

    }

    private void writeToFile(String baseJS) {
        File fileDir = mDelegate.getContext().getFilesDir();
        File combineJs = new File(fileDir, "combine.js");
        FileOutputStream fos = null;
        try {
            if (!combineJs.exists()) {
               combineJs.createNewFile();
            }
             fos = new FileOutputStream(combineJs);
             fos.write(baseJS.getBytes());
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (null != fos) {
                try {
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private void executeDir(File parent, String child) {
        executeDir(parent, child, true);
    }

    private void executeDir(File parent, String child, boolean useBase) {
        File dir = new File(parent, child);
        Map<String, String> pathMaps = new HashMap<>();
        loadFiles(dir, pathMaps);
        for (Map.Entry<String, String> path : pathMaps.entrySet()) {
            if (DEPENDS.contains(path.getKey())) {
                continue;
            }
            executeJs(new File(path.getValue()), useBase);
        }

    }

    public void executeJs(String data, String fileName) {
        int ret = mDelegate.executeJS(data, fileName);
        Log.i(TAG, "ret=" + ret);
    }

    public void executeJs(File dest, boolean useBase) {
        String data = Utils.readJSFile(dest);
        if (useBase) {
            data = baseJS + data + endScript;
        }
        if (!TextUtils.isEmpty(data)) {
            int ret = mDelegate.executeJS(data, dest.getPath());
            if (ret != 0) {
                mFailedPaths.add(dest.getPath());
            }
        } else {
            Log.e(TAG, dest.getPath() + " bytes is null!!!");
        }
    }

    private String makeBaseJS(File file) {
        String script = "";
        Map<String, String> pathMaps = new HashMap<>();
        loadFiles(file, pathMaps);
        for (String depend: DEPENDS) {
            script += Utils.readJSFile(new File(pathMaps.get(depend))) + "\n";
        }
        Log.i(TAG, "baseJS is" + script);
        return script;
    }

    private String makeJS(File file) {
        String script = "";
        Map<String, String> pathMaps = new HashMap<>();
        loadFiles(file, pathMaps);
        for (Map.Entry<String, String> entry : pathMaps.entrySet()) {
            script += Utils.readJSFile(new File(entry.getValue())) + "\n";
        }
        Log.i(TAG, "baseJS is" + script);
        return script;
    }

    private void loadFiles(File root, Map<String, String> paths) {
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
                if (EXCLUDE_SOURCE.contains(root.getName())) {
                    Log.i(TAG, "skip files:" + root.getName());
                    return;
                }
                paths.put(root.getName(), root.getAbsolutePath());
            }
        }
    }

    public void unzipResource() {
        mDelegate.startLoading();
        final File filesDir = mDelegate.getContext().getFilesDir();
        final File testEs5 = new File(filesDir, "es5test.zip");
        if (testEs5.exists()) {
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Utils.unZipAssetsFolder(mDelegate.getContext(), "es5test.zip", filesDir.getAbsolutePath());
                } catch (Exception e) {
                    e.printStackTrace();
                }
                mDelegate.stopLoading();
            }
        }).run();
    }

    private static final String endScript = "ES5Harness.startTesting();\n";

}
