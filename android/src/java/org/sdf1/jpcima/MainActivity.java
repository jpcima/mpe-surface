package org.sdf1.jpcima;

import android.content.*;
import android.os.*;
import android.util.*;
import androidx.core.app.JobIntentService;
import org.qtproject.qt5.android.bindings.QtActivity;

public class MainActivity extends QtActivity
{
    private String TAG = "MainActivity";

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        Intent intent = new Intent(this, MidiService.class);
        intent.putExtra("type", MidiService.START);
        JobIntentService.enqueueWork(this, MidiService.class, MidiService.JOB_ID, intent);
     }
}
