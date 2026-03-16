// MyNativeModule.java

package com.timscodes.fileshare;

import android.content.Intent;
import android.os.Build;
import android.os.Process;
import android.util.DisplayMetrics;
import android.widget.Toast;

import com.facebook.react.bridge.Arguments;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.Promise;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.bridge.WritableMap;
import com.facebook.react.modules.core.DeviceEventManagerModule;
import com.facebook.react.uimanager.IllegalViewOperationException;
import com.facebook.react.bridge.Callback;
//import com.google.android.gms.tasks.OnFailureListener;
//import com.google.android.gms.tasks.OnSuccessListener;
/*
import com.google.mlkit.vision.barcode.BarcodeScanner;
import com.google.mlkit.vision.barcode.BarcodeScannerOptions;
import com.google.mlkit.vision.barcode.BarcodeScanning;
import com.google.mlkit.vision.barcode.common.Barcode;
import com.google.mlkit.vision.common.InputImage;
*/

//import com.google.android.gms.tasks.Task;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;

import java.io.IOException;
import java.util.Map;
import java.util.HashMap;

import android.app.Activity;

import org.json.JSONArray;
import org.json.JSONObject;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Collections;
import java.util.List;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;

import android.Manifest;

import android.provider.Settings;

//import com.fxtf.gx.live.NotificationService;

public class MyNativeModule extends ReactContextBaseJavaModule {
  private ReactApplicationContext mReactContext;

  private static final String DURATION_SHORT_KEY = "SHORT";
  private static final String DURATION_LONG_KEY = "LONG";

  public MyNativeModule(ReactApplicationContext reactContext) {
    super(reactContext);
    mReactContext = reactContext;
  }

  static {
        //System.loadLibrary("native-lib");
        System.loadLibrary("appmodules");
  }
  
  public native String stringFromJNI();
  public native String jniTestWriteFile(String dir);
  public native String jniSetCallback(String packageName, String dir);
  public native String jniJsonCmd(String jsonStr);
  public native String jniMD5Pwd1(String _in);
  public native String jniOnLoad();

  private void sendEvent(String eventName,
        /*@Nullable*/ WritableMap params) {
    mReactContext
            .getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter.class)
            .emit(eventName, params);
  }

    // 使用 ContentResolver 从 content:// URI 获取文件名
    public static String getFileName(Context context, Uri uri) {
        String result = null;

        if (uri.getScheme().equals("content")) {
            //context.getContentResolver().takePersistableUriPermission(uri,
              //      Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            try (Cursor cursor = context.getContentResolver().query(uri, null, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                    result = cursor.getString(nameIndex);
                }
            }
        }

        // 如果不是 content:// URI，可能是文件路径，直接获取最后的文件名
        if (result == null) {
            result = uri.getPath();
            int cut = result.lastIndexOf('/');
            if (cut != -1) {
                result = result.substring(cut + 1);
            }
        }
        return result;
    }

  //jni callback
  public void onTestMessage(int code, String msg) {
      WritableMap params = Arguments.createMap();
      params.putString("cmd", "native");
      params.putString("msg", msg);
      sendEvent("cmd", params);
  }

  @Override
  public String getName() {
    return "MyNativeModule";
  }

  @ReactMethod
  public void MD5Pwd1(String value, Promise promise) {
      String ret = jniMD5Pwd1(value);
      promise.resolve(ret);
  }
  
  @ReactMethod
  public void testWriteFile(Promise promise) {
      //String dir = jniGetExecDir();
      File f = getReactApplicationContext().getFilesDir();

      String dir = jniTestWriteFile(f.toString());
      WritableMap values = Arguments.createMap();
      values.putString("dir", dir);
      //values.putString("dir", f.toString());
      promise.resolve(values);
  }

  @ReactMethod
  public void init(Promise promise) {

    jniOnLoad();

      File f = getReactApplicationContext().getFilesDir();
      String ret = jniSetCallback(getReactApplicationContext().getPackageName(), f.toString());
      WritableMap values = Arguments.createMap();
      values.putString("ret", ret);
      values.putString("filesDir", f.toString());
      values.putString("cacheDir", getReactApplicationContext().getCacheDir().toString());
      values.putString("packageName", getReactApplicationContext().getPackageName());
      promise.resolve(values);
  }
  
  @Override
  public Map<String, Object> getConstants() {
    final Map<String, Object> constants = new HashMap<>();
    constants.put(DURATION_SHORT_KEY, Toast.LENGTH_SHORT);
    constants.put(DURATION_LONG_KEY, Toast.LENGTH_LONG);
    return constants;
  }
  
  /*
	Boolean -> Bool
	Integer -> Number
	Double -> Number
	Float -> Number
	String -> String
	Callback -> function
	ReadableMap -> Object
	ReadableArray -> Array
  */

    public float getDensity() {
        DisplayMetrics metrics = getReactApplicationContext().getResources().getDisplayMetrics();
        return metrics.density;
    }

    @ReactMethod
    public void myToast(String message, int duration) {
        Toast.makeText(getReactApplicationContext(), message, duration).show();
    }

    public static String getIPAddress(boolean useIPv4) {
        JSONObject jsonObject = new JSONObject();
        JSONArray ipList = new JSONArray();
        try {
            List<NetworkInterface> interfaces = Collections.list(NetworkInterface.getNetworkInterfaces());
            for (NetworkInterface intf : interfaces) {
                List<InetAddress> addrs = Collections.list(intf.getInetAddresses());
                for (InetAddress addr : addrs) {
                    if (!addr.isLoopbackAddress()) {
                        String sAddr = addr.getHostAddress();
                        boolean isIPv4 = sAddr.indexOf(':') < 0;

                        if (useIPv4) {
                            if (isIPv4) {
                                ipList.put(sAddr);
                            }
                        } else {
                            if (!isIPv4) {
                                int delim = sAddr.indexOf('%'); // drop ip6 zone suffix
                                sAddr = delim < 0 ? sAddr.toUpperCase() : sAddr.substring(0, delim).toUpperCase();
                                ipList.put(sAddr);
                            }
                        }
                    }
                }
            }
            jsonObject.put("ret", "ok");
            jsonObject.put("allAddress", ipList);
        } catch (Exception ex) {
            // Handle exceptions or print error messages
            //ex.printStackTrace();
            //jsonObject.put("ret", "catch error 1");
        }
        
        return jsonObject.toString(); // Convert JSON Array to String
    }

    @ReactMethod
    public void testCallbackFunc(String testStr, Promise promise) {
        onTestMessage(0,testStr);
        stringFromJNI();
        promise.resolve(testStr);
    }

    @ReactMethod
    public void jsonCmd(String jsonStr, Promise promise) {
        try {
            JSONObject jsonObject = new JSONObject(jsonStr);
            if ("GetLocalIPAddress".equals(jsonObject.getString("cmd"))) {
                String err = getIPAddress(true);
                promise.resolve(err);
                return;    
            }
            else {
/*
                Context context = getReactApplicationContext();
                Uri uri = Uri.parse("content://com.android.providers.downloads.documents/document/raw%3A%2Fstorage%2Femulated%2F0%2FDownload%2Fglobdata.ini");
                //String fileName = getFileName(context, uri);
                File file= new File(uri.getPath());
                String fileName = file.getName();*/

                String err = jniJsonCmd(jsonStr);
                promise.resolve(err);
            }
        } catch (Exception ex) {
            // Handle exceptions or print error messages
            ex.printStackTrace();
        }
    }
/*
    //https://medium.com/@daneallist/implement-qr-code-scanning-from-an-image-in-react-native-0cd12fdaa14e
    @ReactMethod
    public void  scanFromPath(String uri, Promise promise) {
        if (ContextCompat.checkSelfPermission(getReactApplicationContext(),
                Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(getCurrentActivity(),
                    new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                    1001);
            promise.resolve("Permission required");
            return;
        } else {
            //promise.resolve("Permission granted");
        }
        // 1
        BarcodeScannerOptions options =
        new BarcodeScannerOptions.Builder()
        .setBarcodeFormats(
                Barcode.FORMAT_QR_CODE,
                Barcode.FORMAT_AZTEC)
        .build();
        
        // 2
        InputImage image = null;
        try {
            image = InputImage.fromFilePath(getReactApplicationContext(), Uri.parse(uri));
        } catch (IOException e) {
            e.printStackTrace();
            promise.resolve("fail to read image");
            return;
        }

        BarcodeScanner scanner = BarcodeScanning.getClient();
        Task<List<Barcode>> result = scanner.process(image)
        .addOnSuccessListener(new OnSuccessListener<List<Barcode>>() {
            @Override
            public void onSuccess(List<Barcode> barcodes) {
                // Task completed successfully
                // ...
                promise.resolve(barcodes);
            }
        })
        .addOnFailureListener(new OnFailureListener() {
            @Override
            public void onFailure(@NonNull Exception e) {
                // Task failed with an exception
                // ...
                promise.resolve("fail");
            }
        });

        //promise.resolve("end");
    }
    */

    @ReactMethod
    public void getStatusBarHeight(Promise promise) {
      float height = 0;
        int resourceId =getReactApplicationContext().getResources().getIdentifier("status_bar_height","dimen","android");
        if (resourceId > 0) {
            height = getReactApplicationContext().getResources().getDimensionPixelOffset(resourceId) / getDensity();
        }
        promise.resolve(height);
    }

    @ReactMethod
    public void moveToBackground() {
        Activity activity = getCurrentActivity();
        if (activity != null) {
            activity.moveTaskToBack(true);
        }
    }

	@ReactMethod
    public void myExit(/*Promise promise*/) {
      //finishAndRemoveTask(); //after API level 21
      System.exit(0);
      Process.killProcess(Process.myPid());

      //float ret = 0;
      //promise.resolve(ret);
    }
	
    @ReactMethod
    public void getBottomPadding(Promise promise) {
        float height = 0;
        float deviceHeight = 0;
        /*
        int resourceId =getReactApplicationContext().getResources().getIdentifier("design_bottom_navigation_height","dimen","android");
        if (resourceId > 0) {
            height = getReactApplicationContext().getResources().getDimensionPixelOffset(resourceId) / getDensity();
        }
        else {
            resourceId =getReactApplicationContext().getResources().getIdentifier("navigation_bar_height","dimen","android");
            if (resourceId > 0) {
                height = getReactApplicationContext().getResources().getDimensionPixelOffset(resourceId) / getDensity();
            }
        }
         */
        // getRealMetrics is only available with API 17 and +
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            DisplayMetrics metrics = new DisplayMetrics();
            getCurrentActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
            int usableHeight = metrics.heightPixels;
            getCurrentActivity().getWindowManager().getDefaultDisplay().getRealMetrics(metrics);
            int realHeight = metrics.heightPixels;
            if (realHeight > usableHeight)
                height = (realHeight - usableHeight) / getDensity();
            deviceHeight = realHeight / getDensity();
        }
        else {
            DisplayMetrics metrics = new DisplayMetrics();
            getCurrentActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
            int realHeight = metrics.heightPixels;
            deviceHeight = realHeight / getDensity();
        }

        WritableMap values = Arguments.createMap();
        values.putDouble("height", deviceHeight);
        values.putDouble("bottom", height);
        promise.resolve(values);
    }

   @ReactMethod
    public void getAppVersion(Callback successCallback) {
      /*
        try {
            PackageInfo info = getPackageInfo();
            if(info != null){
                successCallback.invoke(info.versionName);
            }else {
                successCallback.invoke("");
            }
        } catch (IllegalViewOperationException e){

        }
        */

      /*
       WritableMap params = Arguments.createMap();
       params.putString("cmd", "native");
       params.putString("msg", "xxx");
       sendEvent("cmd", params);
       */

      //successCallback.invoke(setCallback());
    }

    @ReactMethod
    public void startNotification(String title,String content){
	/*
        Intent serviceIntent = new Intent(getCurrentActivity(), NotificationService.class);
        serviceIntent.putExtra("title",title);
        serviceIntent.putExtra("content",content);

        ContextCompat.startForegroundService(getCurrentActivity(), serviceIntent);
		*/
    }
	
	private PackageInfo getPackageInfo(){
        PackageManager manager = getReactApplicationContext().getPackageManager();
        PackageInfo info = null;
        try{
            info = manager.getPackageInfo(getReactApplicationContext().getPackageName(),0);
            return info;
        }catch (Exception e){
            e.printStackTrace();
        }finally {

            return info;
        }
    }

    @ReactMethod
  public void getDeviceDimensions(Promise promise) {
    try {
        DisplayMetrics realMetrics = new DisplayMetrics();
        DisplayMetrics usableMetrics = new DisplayMetrics();
        
        // Get real (full) screen dimensions
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            getCurrentActivity().getWindowManager().getDefaultDisplay().getRealMetrics(realMetrics);
        } else {
            getCurrentActivity().getWindowManager().getDefaultDisplay().getMetrics(realMetrics);
        }
        
        // Get usable screen dimensions (excluding system bars)
        getCurrentActivity().getWindowManager().getDefaultDisplay().getMetrics(usableMetrics);
        
        float density = getDensity();
        
        // Calculate dimensions in DP
        int realWidthPx = realMetrics.widthPixels;
        int realHeightPx = realMetrics.heightPixels;
        int usableWidthPx = usableMetrics.widthPixels;
        int usableHeightPx = usableMetrics.heightPixels;
        
        float realWidth = realWidthPx / density;
        float realHeight = realHeightPx / density;
        float usableWidth = usableWidthPx / density;
        float usableHeight = usableHeightPx / density;
        
        // Determine current orientation
        boolean isLandscape = realWidthPx > realHeightPx;
        
        // Calculate status bar height
        float statusBarHeight = getStatusBarHeightInternal();
        
        // Calculate navigation bar dimensions
        float navigationBarHeight = 0;
        float navigationBarWidth = 0;
        int forceFsgNavBar = 0;
        boolean hasGestureNavigation = false;
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            if (isLandscape) {
                // In landscape, navigation bar might be on the side
                navigationBarWidth = (realWidthPx - usableWidthPx) / density;
                navigationBarHeight = (realHeightPx - usableHeightPx) / density;
            } else {
                // In portrait, navigation bar is at bottom
                navigationBarHeight = (realHeightPx - usableHeightPx) / density;
                navigationBarWidth = 0;
            }

            int resourceId = getReactApplicationContext().getResources().getIdentifier("navigation_bar_height", "dimen", "android");
            if (resourceId > 0) {
                navigationBarHeight = getReactApplicationContext().getResources().getDimensionPixelOffset(resourceId) / getDensity();
            }

            try {
              forceFsgNavBar = Settings.Secure.getInt(
                      getReactApplicationContext().getContentResolver(), 
                      "force_fsg_nav_bar", 
                      0
                  );
                  /*0 = Default behavior (gesture navigation hides nav bar)
                  1 = Force navigation bar to show even with gestures enabled
                  2 = (varies by device/Samsung version)
                  */
              hasGestureNavigation = isGestureNavigationEnabled();
            }
            catch (Exception e) {
                // Setting not available on this device
                forceFsgNavBar = 0;
            }
        }
        
        // Get system bar insets using WindowInsets (API 20+)
        float leftInset = 0;
        float rightInset = 0;
        float topInset = statusBarHeight;
        float bottomInset = navigationBarHeight;
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT_WATCH) {
            try {
                android.view.View decorView = getCurrentActivity().getWindow().getDecorView();
                if (decorView != null) {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                        android.view.WindowInsets insets = decorView.getRootWindowInsets();
                        if (insets != null) {
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                                // API 28+ - Use DisplayCutout for notches
                                android.view.DisplayCutout cutout = insets.getDisplayCutout();
                                if (cutout != null) {
                                    leftInset = cutout.getSafeInsetLeft() / density;
                                    rightInset = cutout.getSafeInsetRight() / density;
                                    topInset = Math.max(statusBarHeight, cutout.getSafeInsetTop() / density);
                                    bottomInset = Math.max(navigationBarHeight, cutout.getSafeInsetBottom() / density);
                                }
                            }
                            
                            // System window insets
                            leftInset = Math.max(leftInset, insets.getSystemWindowInsetLeft() / density);
                            rightInset = Math.max(rightInset, insets.getSystemWindowInsetRight() / density);
                            topInset = Math.max(topInset, insets.getSystemWindowInsetTop() / density);
                            bottomInset = Math.max(bottomInset, insets.getSystemWindowInsetBottom() / density);
                        }
                    }
                }
            } catch (Exception e) {
                // Fallback to calculated values
            }
        }
        
        // Calculate safe area dimensions
        float safeWidth = realWidth - leftInset - rightInset;
        float safeHeight = realHeight - topInset - bottomInset;
        
        // Build result object
        WritableMap result = Arguments.createMap();

        result.putInt("forceFsgNavBar", forceFsgNavBar);
        result.putBoolean("hasGestureNavigation", hasGestureNavigation);
        
        // Screen dimensions
        WritableMap screen = Arguments.createMap();
        screen.putDouble("width", realWidth);
        screen.putDouble("height", realHeight);
        screen.putInt("widthPixels", realWidthPx);
        screen.putInt("heightPixels", realHeightPx);
        result.putMap("screen", screen);
        
        // Window dimensions (excluding system bars)
        WritableMap window = Arguments.createMap();
        window.putDouble("width", usableWidth);
        window.putDouble("height", usableHeight);
        window.putInt("widthPixels", usableWidthPx);
        window.putInt("heightPixels", usableHeightPx);
        result.putMap("window", window);
        
        // Safe area dimensions
        WritableMap safeArea = Arguments.createMap();
        safeArea.putDouble("width", safeWidth);
        safeArea.putDouble("height", safeHeight);
        result.putMap("safeArea", safeArea);
        
        // System bar heights/widths
        WritableMap systemBars = Arguments.createMap();
        systemBars.putDouble("statusBarHeight", statusBarHeight);
        systemBars.putDouble("navigationBarHeight", navigationBarHeight);
        systemBars.putDouble("navigationBarWidth", navigationBarWidth);
        result.putMap("systemBars", systemBars);
        
        // Current insets (safe area padding)
        WritableMap insets = Arguments.createMap();
        insets.putDouble("top", topInset);
        insets.putDouble("bottom", bottomInset);
        insets.putDouble("left", leftInset);
        insets.putDouble("right", rightInset);
        result.putMap("insets", insets);
        
        // Device info
        WritableMap deviceInfo = Arguments.createMap();
        deviceInfo.putString("orientation", isLandscape ? "landscape" : "portrait");
        deviceInfo.putDouble("density", density);
        deviceInfo.putInt("densityDpi", realMetrics.densityDpi);
        deviceInfo.putString("androidVersion", Build.VERSION.RELEASE);
        deviceInfo.putInt("apiLevel", Build.VERSION.SDK_INT);
        result.putMap("deviceInfo", deviceInfo);
        
        // Portrait specific dimensions (calculated)
        WritableMap portrait = Arguments.createMap();
        if (isLandscape) {
            // Currently in landscape, calculate portrait dimensions
            portrait.putDouble("screenWidth", Math.min(realWidth, realHeight));
            portrait.putDouble("screenHeight", Math.max(realWidth, realHeight));
            portrait.putDouble("statusBarHeight", statusBarHeight);
            portrait.putDouble("navigationBarHeight", getNavigationBarHeightResource());
            
            // Calculate portrait insets from landscape - consider rotation and cutouts
            WritableMap portraitInsets = calculatePortraitInsets(leftInset, rightInset, topInset, bottomInset, density);
            portrait.putDouble("leftInset", portraitInsets.getDouble("left"));
            portrait.putDouble("rightInset", portraitInsets.getDouble("right"));
            portrait.putDouble("topInset", portraitInsets.getDouble("top"));
            portrait.putDouble("bottomInset", portraitInsets.getDouble("bottom"));
        } else {
            // Currently in portrait
            portrait.putDouble("screenWidth", realWidth);
            portrait.putDouble("screenHeight", realHeight);
            portrait.putDouble("statusBarHeight", topInset);
            portrait.putDouble("navigationBarHeight", bottomInset);
            portrait.putDouble("leftInset", leftInset);
            portrait.putDouble("rightInset", rightInset);
            portrait.putDouble("topInset", topInset);
            portrait.putDouble("bottomInset", bottomInset);
        }
        result.putMap("portrait", portrait);
        
        // Landscape specific dimensions (calculated)
        WritableMap landscape = Arguments.createMap();
        if (isLandscape) {
            // Currently in landscape
            landscape.putDouble("screenWidth", realWidth);
            landscape.putDouble("screenHeight", realHeight);
            landscape.putDouble("statusBarHeight", topInset);
            landscape.putDouble("navigationBarHeight", bottomInset);
            landscape.putDouble("leftInset", leftInset);
            landscape.putDouble("rightInset", rightInset);
            landscape.putDouble("topInset", topInset);
            landscape.putDouble("bottomInset", bottomInset);
        } else {
            // Currently in portrait, calculate landscape dimensions
            landscape.putDouble("screenWidth", Math.max(realWidth, realHeight));
            landscape.putDouble("screenHeight", Math.min(realWidth, realHeight));
            landscape.putDouble("statusBarHeight", statusBarHeight);
            
            // Calculate landscape insets from portrait
            WritableMap landscapeInsets = calculateLandscapeInsets(leftInset, rightInset, topInset, bottomInset, density);
            landscape.putDouble("navigationBarHeight", landscapeInsets.getDouble("bottom"));
            landscape.putDouble("leftInset", landscapeInsets.getDouble("left"));
            landscape.putDouble("rightInset", landscapeInsets.getDouble("right"));
            landscape.putDouble("topInset", landscapeInsets.getDouble("top"));
            landscape.putDouble("bottomInset", landscapeInsets.getDouble("bottom"));
        }
        result.putMap("landscape", landscape);
        
        promise.resolve(result);
        
    } catch (Exception e) {
        promise.reject("DIMENSION_ERROR", "Failed to get device dimensions: " + e.getMessage());
    }
  }

  // Helper method to get status bar height from resources
  private float getStatusBarHeightInternal() {
    float height = 0;
    int resourceId = getReactApplicationContext().getResources().getIdentifier("status_bar_height", "dimen", "android");
    if (resourceId > 0) {
        height = getReactApplicationContext().getResources().getDimensionPixelOffset(resourceId) / getDensity();
    }
    return height;
  }

  // Helper method to get navigation bar height from resources
  private float getNavigationBarHeightResource() {
    float height = 0;
    int resourceId = getReactApplicationContext().getResources().getIdentifier("navigation_bar_height", "dimen", "android");
    if (resourceId > 0) {
        height = getReactApplicationContext().getResources().getDimensionPixelOffset(resourceId) / getDensity();
    }
    return height;
  }

  // Helper method to detect gesture navigation
  private boolean isGestureNavigationEnabled() {
    try {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            // Android 10+ method
            int gestureMode = Settings.Secure.getInt(
                getReactApplicationContext().getContentResolver(),
                "navigation_mode",
                0
            );
            return gestureMode == 2; // 2 = gesture navigation
        } else {
            // For older Android versions, check if nav bar height is very small
            float navBarHeight = getNavigationBarHeightResource();
            return navBarHeight < 10; // Likely gesture navigation if very small
        }
    } catch (Exception e) {
        return false;
    }
  }

  // Helper method to calculate portrait insets when currently in landscape
  private WritableMap calculatePortraitInsets(float landscapeLeft, float landscapeRight, 
                                          float landscapeTop, float landscapeBottom, float density) {
    WritableMap insets = Arguments.createMap();
    
    try {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            android.view.View decorView = getCurrentActivity().getWindow().getDecorView();
            if (decorView != null) {
                android.view.WindowInsets windowInsets = decorView.getRootWindowInsets();
                if (windowInsets != null) {
                    android.view.DisplayCutout cutout = windowInsets.getDisplayCutout();
                    if (cutout != null) {
                        int rotation = getCurrentActivity().getWindowManager().getDefaultDisplay().getRotation();
                        
                        // Map landscape insets to portrait based on rotation
                        switch (rotation) {
                            case android.view.Surface.ROTATION_90:
                                // Rotated 90° clockwise: landscape left becomes portrait top
                                insets.putDouble("top", getStatusBarHeightInternal());
                                insets.putDouble("bottom", getNavigationBarHeightResource());
                                insets.putDouble("left", landscapeTop);  // landscape top becomes portrait left
                                insets.putDouble("right", landscapeBottom); // landscape bottom becomes portrait right
                                break;
                            case android.view.Surface.ROTATION_270:
                                // Rotated 90° counter-clockwise: landscape right becomes portrait top
                                insets.putDouble("top", getStatusBarHeightInternal());
                                insets.putDouble("bottom", getNavigationBarHeightResource());
                                insets.putDouble("left", landscapeBottom);  // landscape bottom becomes portrait left
                                insets.putDouble("right", landscapeTop);    // landscape top becomes portrait right
                                break;
                            default:
                                // Default mapping
                                insets.putDouble("top", getStatusBarHeightInternal());
                                insets.putDouble("bottom", getNavigationBarHeightResource());
                                insets.putDouble("left", Math.max(landscapeLeft, 0));
                                insets.putDouble("right", Math.max(landscapeRight, 0));
                                break;
                        }
                        return insets;
                    }
                }
            }
        }
    } catch (Exception e) {
        // Fallback
    }
    
    // Fallback calculations
    insets.putDouble("top", getStatusBarHeightInternal());
    insets.putDouble("bottom", getNavigationBarHeightResource());
    insets.putDouble("left", 0);
    insets.putDouble("right", 0);
    
    return insets;
  }

  // Helper method to calculate landscape insets when currently in portrait
  private WritableMap calculateLandscapeInsets(float portraitLeft, float portraitRight, 
                                           float portraitTop, float portraitBottom, float density) {
    WritableMap insets = Arguments.createMap();
    
    try {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            android.view.View decorView = getCurrentActivity().getWindow().getDecorView();
            if (decorView != null) {
                android.view.WindowInsets windowInsets = decorView.getRootWindowInsets();
                if (windowInsets != null) {
                    android.view.DisplayCutout cutout = windowInsets.getDisplayCutout();
                    if (cutout != null) {
                        // In landscape, portrait left/right often become top/bottom cutouts
                        // Portrait top (status bar area) might have notch that becomes side cutout
                        
                        insets.putDouble("top", getStatusBarHeightInternal());
                        insets.putDouble("bottom", 0); // Usually no bottom nav in landscape
                        
                        // Navigation bar typically goes to right side in landscape
                        float navBarHeight = getNavigationBarHeightResource();
                        insets.putDouble("left", Math.max(portraitLeft, 0));
                        insets.putDouble("right", Math.max(Math.max(portraitRight, navBarHeight), 0));
                        
                        return insets;
                    }
                }
            }
        }
    } catch (Exception e) {
        // Fallback
    }
    
    // Fallback calculations
    insets.putDouble("top", getStatusBarHeightInternal());
    insets.putDouble("bottom", 0);
    insets.putDouble("left", Math.max(portraitLeft, 0));
    insets.putDouble("right", Math.max(portraitRight, getNavigationBarHeightResource()));
    
    return insets;
  }
}
