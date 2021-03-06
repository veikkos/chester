package com.chester.chesterapp;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.Toast;

import java.io.File;
import java.nio.ByteBuffer;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-lib");
    }

    final int readRequestCode = 124;
    final static String TAG = "Chester";
    static java.util.concurrent.atomic.AtomicBoolean paused;
    static java.util.concurrent.atomic.AtomicBoolean destroyed;
    static java.util.concurrent.atomic.AtomicBoolean toggleColors;
    static ChesterView chesterView;
    int screenWidth;
    Thread chesterThread;

    @SuppressLint("ClickableViewAccessibility")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            int result = this.checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE);
            if (result != PackageManager.PERMISSION_GRANTED) {
                try {
                    ActivityCompat.requestPermissions(this,
                            new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                            readRequestCode);
                } catch (Exception e) {
                    e.printStackTrace();
                    throw e;
                }
            }
            else
            {
                openGameSelector(this);
            }
        }
        else
        {
            openGameSelector(this);
        }

        paused = new java.util.concurrent.atomic.AtomicBoolean(true);
        destroyed = new java.util.concurrent.atomic.AtomicBoolean(false);
        toggleColors = new java.util.concurrent.atomic.AtomicBoolean(false);

        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);

        chesterView = findViewById(R.id.chesterView);
        chesterView.setScreenWidth(size.x);

        screenWidth = size.x;

        buttonCallbacks();

        final Context context = this;
        ImageButton gameSelect = findViewById(R.id.game_select);
        gameSelect.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    openGameSelector(context);
                }
                return true;
            }
        });

        ImageButton colorCorrection = findViewById(R.id.colors);
        colorCorrection.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    toggleColors.set(true);
                }
                return true;
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        paused.set(false);

        setFullScreen();

        ActionBar bar = getSupportActionBar();
        if (bar != null) {
            bar.hide();
        }

        ViewGroup.LayoutParams params = chesterView.getLayoutParams();
        params.height = screenWidth * 144 / 160;
        chesterView.requestLayout();
    }

    @Override
    protected void onPause() {
        paused.set(true);
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        stopChesterIfRunning();
        super.onDestroy();
    }

    private void stopChesterIfRunning() {
        if(chesterThread != null) {
            paused.set(true);
            destroyed.set(true);
            try {
                chesterThread.join();
            } catch (InterruptedException ignored) {}
            chesterThread = null;
            uninitChester();
            paused.set(false);
            destroyed.set(false);
        }
    }

    void setFullScreen() {
        View decorView = getWindow().getDecorView();
        int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN;
        decorView.setSystemUiVisibility(uiOptions);
    }

    private Thread createChesterRunningThread() {
        return new Thread(){
            public void run(){
                Log.i(TAG, "Initialized Chester");
                int ret = 0;
                do {
                    if (paused.get()) {
                        saveChester();
                        if (destroyed.get()) {
                            ret = -1;
                        } else {
                            try {
                                Thread.sleep(350);
                            } catch (InterruptedException ignored) {
                            }
                        }
                    } else {
                        ret = runChester();
                        if (toggleColors.get()) {
                            toggleColors.set(false);
                            toggleColorCorrection();
                        }
                    }
                } while(ret == 0);
            }
        };
    }

    private void openGameSelector(final Context context)
    {
        File mPath = new File(Environment.getExternalStorageDirectory() + "//Download//");
        FileDialog fileDialog = new FileDialog(this, mPath, ".gb");
        fileDialog.addFileListener(new FileDialog.FileSelectedListener() {
            public void fileSelected(File file) {
                stopChesterIfRunning();

                Log.d(getClass().getName(), "selected file " + file.toString());
                if (initChester(file.getAbsolutePath(), getFilesDir().getAbsolutePath() + "/"))
                {
                    chesterThread = createChesterRunningThread();
                    chesterThread.start();
                }
                else
                {
                    CharSequence text = "Couldn't init Chester";
                    int duration = Toast.LENGTH_SHORT;

                    Toast toast = Toast.makeText(context, text, duration);
                    toast.show();
                }

                setFullScreen();
            }
        });
        fileDialog.showDialog();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        if (requestCode == readRequestCode) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                openGameSelector(this);
            } else {
                // permission denied
                finish();
            }
        }
    }

    @SuppressWarnings("unused")
    public static void renderCallback(ByteBuffer buffer){
        int bytes = buffer.remaining();
        byte[] imageBytes = new byte[bytes];
        buffer.get(imageBytes);
        synchronized (chesterView.getImageLock()) {
            Bitmap bm = Bitmap.createBitmap(256, 144, Bitmap.Config.ARGB_8888);
            bm.copyPixelsFromBuffer(ByteBuffer.wrap(imageBytes));
            chesterView.update(bm);
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    public void buttonCallbacks() {
        Button a = findViewById(R.id.a);
        a.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    setKeyA(true);
                    return true;
                } else if (event.getAction() == MotionEvent.ACTION_UP) {
                    setKeyA(false);
                    return true;
                }
                return false;
            }
        });

        Button b = findViewById(R.id.b);
        b.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    setKeyB(true);
                    return true;
                } else if (event.getAction() == MotionEvent.ACTION_UP) {
                    setKeyB(false);
                    return true;
                }
                return false;
            }
        });

        Button start = findViewById(R.id.start);
        start.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    setKeyStart(true);
                    return true;
                } else if (event.getAction() == MotionEvent.ACTION_UP) {
                    setKeyStart(false);
                    return true;
                }
                return false;
            }
        });

        Button select = findViewById(R.id.select);
        select.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    setKeySelect(true);
                    return true;
                } else if (event.getAction() == MotionEvent.ACTION_UP) {
                    setKeySelect(false);
                    return true;
                }
                return false;
            }
        });

        Button up = findViewById(R.id.up);
        up.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    setKeyUp(true);
                    return true;
                } else if (event.getAction() == MotionEvent.ACTION_UP) {
                    setKeyUp(false);
                    return true;
                }
                return false;
            }
        });

        Button down = findViewById(R.id.down);
        down.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    setKeyDown(true);
                    return true;
                } else if (event.getAction() == MotionEvent.ACTION_UP) {
                    setKeyDown(false);
                    return true;
                }
                return false;
            }
        });

        Button left = findViewById(R.id.left);
        left.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    setKeyLeft(true);
                    return true;
                } else if (event.getAction() == MotionEvent.ACTION_UP) {
                    setKeyLeft(false);
                    return true;
                }
                return false;
            }
        });

        Button right = findViewById(R.id.right);
        right.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if(event.getAction() == MotionEvent.ACTION_DOWN) {
                    setKeyRight(true);
                    return true;
                } else if (event.getAction() == MotionEvent.ACTION_UP) {
                    setKeyRight(false);
                    return true;
                }
                return false;
            }
        });
    }

    public native boolean initChester(String rom, String savePath);
    public native void uninitChester();

    public native int runChester();

    public native void saveChester();

    public native void setKeyA(boolean pressed);
    public native void setKeyB(boolean pressed);
    public native void setKeyStart(boolean pressed);
    public native void setKeySelect(boolean pressed);
    public native void setKeyUp(boolean pressed);
    public native void setKeyDown(boolean pressed);
    public native void setKeyLeft(boolean pressed);
    public native void setKeyRight(boolean pressed);

    public native void toggleColorCorrection();
}
