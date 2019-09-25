package com.example.realsense_app;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.ToggleButton;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (ContextCompat.checkSelfPermission(getApplicationContext(), Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);
        }

        mAllViews = findViewById(R.id.all_views);

        mDefaultStream = findViewById(R.id.default_stream);
        mDefaultStream.setEnabledForUI(false);

        mLogView = findViewById(R.id.logView);


        mButtonAddView = findViewById(R.id.buttonAddView);
        mButtonAddView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                StreamSurface newView = new StreamSurface(getApplicationContext());
                newView.setOnUpdateStreamProfileListListener(mStreamSurfaceUpdateListener);
                newView.setEnabledForUI(mSwitchButtonPower.isChecked());
                mOtherStreams.add(newView);
                mAllViews.addView(newView, mOtherStreams.size());
            }
        });
        mButtonOCC = findViewById(R.id.buttonOCC);
        mButtonOCC.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mLogView.setText(logMessage());
                nOnChipCalibration();
                mLogView.setText(logMessage());
            }
        });

        mSwitchButtonPower = findViewById(R.id.toggleButtonPower);
        mSwitchButtonPower.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    mDefaultStream.setEnabledForUI(true);
                    for (StreamSurface v: mOtherStreams) {
                        v.setEnabledForUI(true);
                    }

                    init();

                    if (mDefaultStream.isStreamEnabledByDefault()) {
                        mDefaultStream.setStreamEnabled(true);
                    }

                    mLogView.setText(logMessage());

                    mSwitchButtonPlay.setEnabled(true);
                } else {
                    mSwitchButtonPlay.setEnabled(false);

                    cleanup();

                    mDefaultStream.setStreamEnabled(false);
                    for (StreamSurface v: mOtherStreams) {
                        v.setStreamEnabled(false);
                    }

                    mDefaultStream.setEnabledForUI(false);
                    for (StreamSurface v: mOtherStreams) {
                        v.setEnabledForUI(false);
                    }
                }
            }
        });

        mSwitchButtonPlay = findViewById(R.id.toggleButtonPlay);
        mSwitchButtonPlay.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    mSwitchButtonPower.setEnabled(false);

                    Play();
                } else {
                    Stop();

                    mSwitchButtonPower.setEnabled(true);
                }
            }
        });

        mDefaultStream.setOnUpdateStreamProfileListListener(mStreamSurfaceUpdateListener);
    }

    private void Play() {
        EnableStream(mDefaultStream);
        for (StreamSurface v: mOtherStreams) {
            EnableStream(v);
        }

        play();
    }
    private void Stop() {
        stop();
    }

    private void EnableStream(StreamSurface surface) {
        if (!surface.isStreamEnable()) return;

        StreamSurface.StreamConfig config = surface.getCurrentConfig();
        int type =  rs2.StreamFromString(config.mType);
        //surface.getStreamView().getHolder().getSurface();
        Log.d("MAINACTIVITY", String.format("Dev: %d Stream: %s w %d h %d fps %d Format: %s",
                config.mDevice, config.mType, config.mWidth, config.mHeight, config.mFPS, config.mFormat));
        enableStream(config.mDevice,
                rs2.StreamFromString(config.mType),
                config.mWidth, config.mHeight, config.mFPS,
                rs2.FormatFromString(config.mFormat),
                surface.getStreamView().getHolder().getSurface()
                );
    }

    StreamSurface.OnUpdateStreamProfileListListener mStreamSurfaceUpdateListener = new StreamSurface.OnUpdateStreamProfileListListener() {

        @Override
        public String onUpdateStreamProfileList(StreamSurface v, String device, String streamName, String resolution, String sfps) {
            Log.d("RS", "onUpdateStreamProfileList DEV:" + device + " STREAM:" + streamName + " RES:" + resolution + " FPS:" + sfps);
            int deviceIdx = -1; int width = -1; int height = -1; int fps = -1;
            if (device != null) {
                deviceIdx = Integer.parseInt(device);
            }
            if (resolution != null) {
                String[] res = resolution.split("x");
                if (res.length == 2) {
                    width = Integer.parseInt(res[0]);
                    height = Integer.parseInt(res[1]);
                }
            }

            if (sfps!= null && !sfps.isEmpty()) {
                fps = Integer.parseInt(sfps);
            }
            return MainActivity.getProfileList(deviceIdx, streamName, width, height, fps);
        }
    };

    LinearLayout mAllViews;

    StreamSurface mDefaultStream;
    List<StreamSurface> mOtherStreams = new ArrayList<StreamSurface>();

    TextView mLogView;

    Button mButtonAddView;
    Button mButtonOCC;
    ToggleButton mSwitchButtonPower;
    ToggleButton mSwitchButtonPlay;

    public native static void init();
    public native static void cleanup();
    public native static void enableStream(int device, int stream, int width, int height, int fps, int format, Surface surface);
    public native static void play();
    public native static void stop();
    public native static void nOnChipCalibration();

    public native static String getProfileList(int device, String type, int width, int height, int fps);

    public native static String logMessage();
}
