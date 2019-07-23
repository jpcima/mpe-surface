package org.sdf1.jpcima;

import android.content.*;
import android.util.*;
import androidx.core.app.*;
import java.io.*;

public class MidiService extends JobIntentService {
    private static String TAG = "MidiService";

    public static final int JOB_ID = 1000;

    public static final int START = 0;
    public static final int SEND_MIDI = 1;

    private static MidiDeviceInterface midi = null;

    @Override
    public void onCreate()
    {
        super.onCreate();
        Log.i(TAG, "Create service: " + this);

        if (midi == null)
            midi = new MidiDeviceInterface(getApplicationContext());
    }

    @Override
    protected void onHandleWork(Intent intent)
    {
        Log.d(TAG, String.format("Handle intent: %s", intent.toString()));

        int type = intent.getIntExtra("type", -1);
        switch (type) {
        case START:
            Log.d(TAG, "Open MIDI service");
            midi.openInputPort();
            break;

        case SEND_MIDI:
            Log.d(TAG, "Send MIDI message");
            midi.sendMidiMessage(intent.getByteArrayExtra("data"), intent.getDoubleExtra("time", 0.0));
            break;

        default:
            break;
        }
    }
}
