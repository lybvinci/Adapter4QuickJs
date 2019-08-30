package com.lybvinci.adapter4quickjs;

import android.content.Context;

public interface JSDelegate {
    Context getContext();
    int executeJS(String data, String path);
    void startLoading();
    void stopLoading();
}
