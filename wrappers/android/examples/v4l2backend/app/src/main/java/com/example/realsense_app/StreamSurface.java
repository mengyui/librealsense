package com.example.realsense_app;

import android.content.Context;
import android.content.res.TypedArray;
import android.support.annotation.Nullable;
import android.support.constraint.ConstraintLayout;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.Spinner;

import java.util.Locale;

public class StreamSurface extends FrameLayout {

    public interface OnUpdateStreamProfileListListener {
        String onUpdateStreamProfileList(StreamSurface v, String device, String streamName, String resolution, String fps);
    }
    OnUpdateStreamProfileListListener mOnUpdateStreamProfileListListener;
    public void setOnUpdateStreamProfileListListener(OnUpdateStreamProfileListListener listener) {
        if (mOnUpdateStreamProfileListListener == null) {
            if (mStreamEnabled.isChecked()) {
                String list = listener.onUpdateStreamProfileList(
                        StreamSurface.this,
                        mStreamDevice.getSelectedItem().toString(),
                        mStreamTypeList.getSelectedItem().toString(),
                        null,
                        null);
                if (!list.isEmpty())
                    setStreamResolutionList(list);
            }
        }
        mOnUpdateStreamProfileListListener = listener;
    }

    public StreamSurface(Context context) {
        this(context, null);
    }

    public StreamSurface(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public StreamSurface(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        init(context, attrs, defStyleAttr);
    }

    private void loadDefault(AttributeSet attrs, int defStyle) {
        if (attrs == null) return;

        mDefaultConfig = new StreamConfig();
        // Load attributes
        final TypedArray a = getContext().obtainStyledAttributes(
                attrs, R.styleable.StreamSurface, defStyle, 0);

        mDefaultConfig.mDevice = a.getInt(R.styleable.StreamSurface_DefaultDevice, 0);
        mDefaultConfig.mType = a.getString(R.styleable.StreamSurface_DefaultStream);
        mDefaultConfig.mFormat = a.getString(R.styleable.StreamSurface_DefaultFormat);
        mDefaultConfig.mWidth = a.getInt(R.styleable.StreamSurface_DefaultWidth, 640);
        mDefaultConfig.mHeight = a.getInt(R.styleable.StreamSurface_DefaultHeight, 480);
        mDefaultConfig.mFPS = a.getInt(R.styleable.StreamSurface_DefaultFPS, 30);
        mDefaultEnabled = a.getBoolean(R.styleable.StreamSurface_DefaultEnable, false);

        a.recycle();
    }

    private void init(Context context, AttributeSet attrs, int defStyle) {
        if (attrs != null) {
            loadDefault(attrs, defStyle);
        }

        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        inflater.inflate(R.layout.stream_surface, this);

        mStreamEnabled = findViewById(R.id.checkBoxEnabled);
        mStreamDevice = findViewById(R.id.spinnerDevice);
        mStreamTypeList = findViewById(R.id.spinnerStream);
        mStreamResolution = findViewById(R.id.spinnerResolution);
        mStreamFPS = findViewById(R.id.spinnerFPS);
        mStreamFormat = findViewById(R.id.spinnerFormat);
        mStreamView = findViewById(R.id.surfaceView);

        String device = String.format(Locale.getDefault(),"%d", mDefaultConfig.mDevice);
        String resolution = String.format(Locale.getDefault(), "%dx%d", mDefaultConfig.mWidth, mDefaultConfig.mHeight);
        String fps = String.format(Locale.getDefault(), "%d", mDefaultConfig.mFPS);

        initSpinner(context, mStreamDevice, device, device);
        initSpinner(context, mStreamTypeList, "Depth,Color,Infrared", mDefaultConfig.mType);
        initSpinner(context, mStreamFormat, mDefaultConfig.mFormat, mDefaultConfig.mFormat);
        initSpinner(context, mStreamResolution, resolution, resolution);
        initSpinner(context, mStreamFPS, fps, fps);

        ViewGroup.LayoutParams params = mStreamView.getLayoutParams();
        params.width = mDefaultConfig.mWidth;
        params.height = mDefaultConfig.mHeight;
        mStreamView.setLayoutParams(params);

        mStreamEnabled.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    if (mOnUpdateStreamProfileListListener != null) {
                        String list = mOnUpdateStreamProfileListListener.onUpdateStreamProfileList(
                                StreamSurface.this,
                                null,
                                "",//mStreamTypeList.getSelectedItem().toString(),
                                null,
                                null);
                        Log.d("RS", "mStreamEnabled onCheckedChanged L: " + list);

                        setStreamDeviceList(list);
                    }
                }
            }
        });

        mStreamDevice.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {

            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                if (!mStreamEnabled.isEnabled()) return;
                Log.d("RS", mStreamDevice.getSelectedItem().toString());
                if (mOnUpdateStreamProfileListListener != null) {
                    String list = mOnUpdateStreamProfileListListener.onUpdateStreamProfileList(
                            StreamSurface.this,
                            mStreamDevice.getSelectedItem().toString(),
                            mStreamTypeList.getSelectedItem().toString(),
                            null,
                            null);
                    Log.d("RS", "mStreamDevice onItemSelected " + mStreamTypeList.getSelectedItem().toString() + " L:" + list);

                    setStreamResolutionList(list);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {
                Log.d("RS", "mStreamDevice onNothingSelected");
            }
        });
        mStreamTypeList.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                if (!mStreamEnabled.isEnabled()) return;
                if (mOnUpdateStreamProfileListListener != null) {
                    String list = mOnUpdateStreamProfileListListener.onUpdateStreamProfileList(
                            StreamSurface.this,
                            mStreamDevice.getSelectedItem().toString(),
                            mStreamTypeList.getSelectedItem().toString(),
                            null,
                            null);
                    Log.d("RS", "mStreamTypeList onItemSelected " + mStreamTypeList.getSelectedItem().toString() + " L:" + list);

                    setStreamResolutionList(list);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {
                Log.d("RS", "mStreamTypeList onNothingSelected");
            }
        });
        mStreamResolution.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {

            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (!mStreamEnabled.isEnabled()) return;
                if (mOnUpdateStreamProfileListListener != null) {
                    String list = mOnUpdateStreamProfileListListener.onUpdateStreamProfileList(
                            StreamSurface.this,
                            mStreamDevice.getSelectedItem().toString(),
                            mStreamTypeList.getSelectedItem().toString(),
                            mStreamResolution.getSelectedItem().toString(),
                            null);
                    Log.d("RS", "mStreamResolution onItemSelected " + mStreamTypeList.getSelectedItem().toString() + " L:" + list);

                    setStreamFPSList(list);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                Log.d("RS", "mStreamResolution onNothingSelected");
            }
        });

        mStreamFPS.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (!mStreamEnabled.isEnabled()) return;
                if (mOnUpdateStreamProfileListListener != null) {
                    String list = mOnUpdateStreamProfileListListener.onUpdateStreamProfileList(
                            StreamSurface.this,
                            mStreamDevice.getSelectedItem().toString(),
                            mStreamTypeList.getSelectedItem().toString(),
                            mStreamResolution.getSelectedItem().toString(),
                            mStreamFPS.getSelectedItem().toString());
                    Log.d("RS", "mStreamFPS onItemSelected " + mStreamTypeList.getSelectedItem().toString() + " L:" + list);

                    setStreamFormatList(list);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                Log.d("RS", "mStreamFPS onNothingSelected");
            }
        });

    }

    private void initSpinner(Context context, Spinner spinner, @Nullable String list, String defValue) {
        if (list == null) return;

        String[] _list = list.split(",");
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(context, android.R.layout.simple_spinner_dropdown_item, _list);
        spinner.setAdapter(adapter);

        int defPosition = adapter.getPosition(defValue);
        if (defPosition >= 0) {
            spinner.setSelection(defPosition);
        }
    }

    private void getDeviceID() {
        String device = mStreamDevice.getSelectedItem().toString();
        mCurrentConfig.mDevice = Integer.parseInt(device);
    }
    private void getWidthAndHeightFromUI() {
        String resolution = mStreamResolution.getSelectedItem().toString();
        String[] _list = resolution.split("x");
        if (_list.length == 2) {
            mCurrentConfig.mWidth = Integer.parseInt(_list[0]);
            mCurrentConfig.mHeight = Integer.parseInt(_list[1]);
        } else {
            mCurrentConfig.mWidth = mDefaultConfig.mWidth;
            mCurrentConfig.mHeight = mDefaultConfig.mHeight;
        }
    }
    private void getFPSFromUI() {
        String fps = mStreamFPS.getSelectedItem().toString();
        if (fps.isEmpty()) {
            mCurrentConfig.mFPS = mDefaultConfig.mFPS;
        } else {
            mCurrentConfig.mFPS = Integer.parseInt(fps);
        }
    }

    public String getCurrentResolution() {
        return mStreamResolution.getSelectedItem().toString();
    }
    public String getCurrentFPS() {
        return mStreamFPS.getSelectedItem().toString();
    }
    public void setStreamDeviceList(String list) {
        initSpinner(mStreamDevice.getContext(), mStreamDevice, list, "0");
    }
    public void setStreamResolutionList(String list) {
        initSpinner(mStreamResolution.getContext(), mStreamResolution, list, String.format(Locale.getDefault(), "%dx%d", mDefaultConfig.mWidth, mDefaultConfig.mHeight));
    }
    public void setStreamFPSList(String list) {
        initSpinner(mStreamFPS.getContext(), mStreamFPS, list, String.format(Locale.getDefault(), "%d", mDefaultConfig.mFPS));
    }
    public void setStreamFormatList(String list) {
        initSpinner(mStreamFormat.getContext(), mStreamFormat, list, mDefaultConfig.mFormat);
    }

    public boolean isStreamEnable() {
        return mStreamEnabled.isChecked();
    }

    public boolean isStreamEnabledByDefault() {
        return mDefaultEnabled;
    }

    public void setStreamEnabled(boolean flag) {
        mStreamEnabled.setChecked(flag);
    }

    public void setEnabledForUI(boolean flag) {
        mStreamEnabled.setEnabled(flag);
    }

    public StreamConfig getCurrentConfig() {
        getDeviceID();
        mCurrentConfig.mType = mStreamTypeList.getSelectedItem().toString();
        mCurrentConfig.mFormat = mStreamFormat.getSelectedItem().toString();
        getWidthAndHeightFromUI();
        getFPSFromUI();

        ViewGroup.LayoutParams params = mStreamView.getLayoutParams();
        params.width = mCurrentConfig.mWidth;
        params.height = mCurrentConfig.mHeight;
        mStreamView.setLayoutParams(params);

        return mCurrentConfig;
    }

    public SurfaceView getStreamView() {
        return mStreamView;
    }

    boolean mDefaultEnabled;
    static StreamConfig mDefaultConfig;
    StreamConfig mCurrentConfig = new StreamConfig();

    CheckBox mStreamEnabled;
    Spinner mStreamDevice;
    Spinner mStreamTypeList;
    Spinner mStreamResolution;
    Spinner mStreamFPS;
    Spinner mStreamFormat;

    SurfaceView mStreamView;

    public class StreamConfig {
        public int mDevice;
        public String mType;
        public String mFormat;
        public int mWidth;
        public int mHeight;
        public int mFPS;
    }
}
