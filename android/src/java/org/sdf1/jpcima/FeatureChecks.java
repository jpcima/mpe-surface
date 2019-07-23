package org.sdf1.jpcima;

import android.content.*;
import android.content.pm.*;

public class FeatureChecks
{
    public static boolean deviceSupportsMidiPorts(Context context)
    {
        PackageManager pm = context.getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_MIDI);
    }

    public static boolean deviceSupportsMultiTouchDistinct(Context context)
    {
        PackageManager pm = context.getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN_MULTITOUCH_DISTINCT);
    }

    public static boolean deviceSupportsMultiTouchJazzHand(Context context)
    {
        PackageManager pm = context.getPackageManager();
        return pm.hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN_MULTITOUCH_JAZZHAND);
    }

    // public static boolean deviceSupportsTouchPressure(Context context)
    // {
    //     PackageManager pm = context.getPackageManager();
    //     return pm.hasSystemFeature(PackageManager.);
    // }
}
