package com.chester.chesterapp;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

public class ChesterView extends View {

    public ChesterView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public ChesterView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public ChesterView(Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    final static String TAG = "ChesterView";

    final static Object imageLock = new Object();
    private Bitmap image;
    private int width;

    public void setScreenWidth(int width) {
        this.width = width;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        synchronized (imageLock) {
            if (image != null) {
                canvas.save();
                float scale = (float)width / 160;
                canvas.scale(scale, scale);
                canvas.drawBitmap(image, 0, 0, null);
                canvas.restore();
            } else {
                Log.i(TAG, "onDraw null");
            }
        }

        super.onDraw(canvas);
    }

    public void update(Bitmap bm) {
        synchronized (imageLock) {
            if (image != null) {
                image.recycle();
            }
            image = bm;
        }
        postInvalidate();
    }

    public Object getImageLock() {
        return imageLock;
    }
}
