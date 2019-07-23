package org.sdf1.jpcima;

import android.content.*;
import android.content.pm.*;
import android.media.midi.*;
import android.os.*;
import android.util.*;
import java.io.*;

class MidiDeviceInterface {
    private static final String TAG = "MidiDeviceInterface";

    private Context context = null;
    private MidiInputPort inPort = null;
    private String inDevName = null;
    private String inPortName = null;

    public MidiDeviceInterface(Context context)
    {
        this.context = context;
    }

    public void closePort() throws IOException
    {
        if (inPort == null)
            return;

        Log.i(TAG, "Close port: " + inPort);

        inPort.close();
        inPort = null;
    }

    public void openInputPort()
    {
        try { closePort(); } catch (IOException ex) {}

        Log.i(TAG, "Open port");

        EndpointInfo ep = getInputEndpoint();
        if (ep == null) {
            Log.e(TAG, "MIDI input endpoint not found");
            return;
        }

        openInputEndpoint(ep);
    }

    public void changeInputPort(String dev, String port)
    {
        inDevName = dev;
        inPortName = port;
        openInputPort();
    }

    public void sendMidiMessage(byte[] message, double timeDelta)
    {
        Log.d(TAG, "Send MIDI to port: " + inPort);

        if (inPort == null)
            return;

        /* TODO: time delta */
        try {
            inPort.send(message, 0, message.length);
        }
        catch (IOException ex) {
            Log.e(TAG, "Cannot send MIDI message", ex);
        }
    }

    private MidiManager getMidiManager()
    {
        PackageManager pm = context.getPackageManager();
        if (!pm.hasSystemFeature(PackageManager.FEATURE_MIDI))
            return null;
        return (MidiManager)context.getSystemService(Context.MIDI_SERVICE);
    }

    private class EndpointInfo {
        MidiDeviceInfo device;
        MidiDeviceInfo.PortInfo port;
    }

    private EndpointInfo getInputEndpoint()
    {
        return findEndpoint(inDevName, inPortName, MidiDeviceInfo.PortInfo.TYPE_INPUT);
    }

    private EndpointInfo findEndpoint(String devName, String portName, int type)
    {
        MidiManager mgr = getMidiManager();
        if (mgr == null)
            return null;

        EndpointInfo ep = null;
        MidiDeviceInfo[] infos = mgr.getDevices();

        for (int i = 0, n = infos.length; i < n && ep == null; ++i) {
            MidiDeviceInfo dev = infos[i];
            MidiDeviceInfo.PortInfo[] ports = dev.getPorts();

            if (devName != null && !devName.equals(dev.getProperties().getCharSequence(MidiDeviceInfo.PROPERTY_NAME)))
                continue;

            for (int j = 0, m = ports.length; j < m && ep == null; ++j) {
                MidiDeviceInfo.PortInfo port = ports[j];

                if (port.getType() != type)
                    continue;

                if (portName == null || portName.equals(port.getName())) {
                    ep = new EndpointInfo();
                    ep.device = dev;
                    ep.port = port;
                }
            }
        }

        return ep;
    }

    private void openInputEndpoint(final EndpointInfo ep)
    {
        MidiManager mgr = getMidiManager();
        if (mgr == null)
            return;

        mgr.openDevice(ep.device, new MidiManager.OnDeviceOpenedListener() {
                @Override
                public void onDeviceOpened(MidiDevice device) {
                    if (device == null) {
                        Log.e(TAG, "Could not open device: " + ep.device);
                    } else {
                        Log.i(TAG, "Opened device: " + ep.device);
                        MidiInputPort port = device.openInputPort(ep.port.getPortNumber());
                        if (port == null) {
                            Log.e(TAG, "Could not open port: " + ep.port);
                            return;
                        }
                        Log.i(TAG, "Opened MIDI port: " + port);
                        inPort = port;
                    }
                }
            },
            new Handler(Looper.getMainLooper()));
    }
}
