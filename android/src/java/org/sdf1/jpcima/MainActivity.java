package org.sdf1.jpcima;

import android.content.*;
import android.os.*;
import android.util.*;
import org.qtproject.qt5.android.bindings.QtActivity;

public class MainActivity extends QtActivity
{
    private String TAG = "MainActivity";

    private static MidiDeviceInterface midi = null;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        Log.i(TAG, "Create activity: " + this);

        if (midi == null) {
            midi = new MidiDeviceInterface(getApplicationContext());
            midi.openInputPort();
        }

        super.onCreate(savedInstanceState);
     }

    public static MidiDeviceInterface getMidiDeviceInterface()
    {
        return midi;
    }
}
