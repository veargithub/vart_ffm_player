package com.example.myndkapplication2;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class VipFrameLayout extends FrameLayout {

    private boolean isVip = false;

    public VipFrameLayout(@NonNull Context context) {
        super(context);
    }

    public VipFrameLayout(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public VipFrameLayout(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void setVip(boolean isVip) {
        this.isVip = isVip;
        requestLayout();
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        int w = right - left;
        int h = bottom - top;
        int padding = isVip ? (int)(w * 0.1f) : 0;
        this.setPadding(padding, padding, padding, padding);
        super.onLayout(changed, left, top, right, bottom);
        Log.d(">>>>", padding + " " + left + " " + top + " " + right + " " + bottom);
    }
}
