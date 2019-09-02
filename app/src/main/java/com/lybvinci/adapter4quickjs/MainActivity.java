package com.lybvinci.adapter4quickjs;

import androidx.annotation.MainThread;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MainActivity extends AppCompatActivity implements JSDelegate{

    private static final String TAG = "LYNX";

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("quickjs");
    }


    private ProgressBar progressBar;

    private TestES5 mTestES5;
    private TestES6 mTestES6;
    private EditText mScriptInput;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // Example of a call to a native method
        mScriptInput = findViewById(R.id.js_script);

        progressBar = findViewById(R.id.progress);
        mTestES5 = new TestES5(this);
        mTestES6 = new TestES6(this);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        nativeDestroy();
    }


    public void runES6Test(View view) {
        mTestES6.runTest();
    }

    public void unzipES6Resource(View view) {
        mTestES6.unzipResource();
    }

    public void runES5Test(View view) {
        mTestES5.runTest();
    }

    public void unzipES5Resource(View view) {
        mTestES5.unzipResource();
    }



    public void runJSScript(View view) {
        String script = mScriptInput.getText().toString();
        if (TextUtils.isEmpty(script)) {
            Toast.makeText(this, "请输入js！", Toast.LENGTH_SHORT).show();
            return;
        }
        nativeTestScript(script);
    }



    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String nativeInitJSEngine();

    public native int nativeRunTest(String jsData, String name);

    public native int nativeRunCleanTest(String jsData, String name);

    public native void nativeDestroy();

    public native void nativeTestScript(String script);

    public native void nativeTestMemory(String jsData, int count);

    @Override
    public Context getContext() {
        return this;
    }

    @Override
    public int executeJS(String data, String path) {
        return nativeRunCleanTest(data, path);
    }

    @Override
    public void startLoading() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                progressBar.setVisibility(View.VISIBLE);
            }
        });
    }

    @Override
    public void stopLoading() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                progressBar.setVisibility(View.GONE);
            }
        });
    }


    public void testMemroy1(View view) {
        nativeTestMemory(Utils.readAssetsFile(this,"combined.js"), 1);
    }

    public void testMemroy10(View view) {
        nativeTestMemory(Utils.readAssetsFile(this,"testmemory.js"), 10);
    }
}
