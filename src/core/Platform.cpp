#include "Platform.h"
#include "IHttpClient.h"
#include "raylib.h"
#include "Config.h"
#include "EnvLoader.h"
#include "FileAssetProvider.h"
#include <cstdlib>

#if defined(__ANDROID__)
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <jni.h>
#include "AndroidAssetProvider.h"
#include "CurlHttpClient.h"
extern "C" struct android_app* GetAndroidApp(void);
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include "EmscriptenClient.h"
#elif defined(THEWORD_HAS_HTTP)
#include "CurlHttpClient.h"
#endif

namespace theword::core { namespace platform {

Info Init(const char* title) {
    Info info;

#if defined(__ANDROID__)
    android_app* app = GetAndroidApp();
    int density = AConfiguration_getDensity(app->config);
    info.dpiScale = (density > 0) ? (float)density / 160.0f : 1.0f;
    if (info.dpiScale < 1.0f) info.dpiScale = 1.0f;
    if (info.dpiScale > 4.0f) info.dpiScale = 4.0f;
    InitWindow(0, 0, title);

    {
        JNIEnv* env = nullptr;
        app->activity->vm->AttachCurrentThread(&env, nullptr);

        jclass resClass = env->FindClass("android/content/res/Resources");
        jmethodID getSystem = env->GetStaticMethodID(
            resClass, "getSystem", "()Landroid/content/res/Resources;");
        jobject res = env->CallStaticObjectMethod(resClass, getSystem);

        jmethodID getIdentifier = env->GetMethodID(
            resClass, "getIdentifier",
            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");
        jstring navName = env->NewStringUTF("navigation_bar_height");
        jstring defType = env->NewStringUTF("dimen");
        jstring defPkg = env->NewStringUTF("android");
        jint navId = env->CallIntMethod(res, getIdentifier, navName, defType, defPkg);

        jmethodID getDimPixelSize = env->GetMethodID(
            resClass, "getDimensionPixelSize", "(I)I");
        info.bottomInset = navId > 0
            ? env->CallIntMethod(res, getDimPixelSize, navId)
            : 0;

        env->DeleteLocalRef(navName);
        env->DeleteLocalRef(defType);
        env->DeleteLocalRef(defPkg);
        env->DeleteLocalRef(res);
        app->activity->vm->DetachCurrentThread();
    }
#else
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(config::WINDOW_WIDTH, config::WINDOW_HEIGHT, title);
    Vector2 dpi = GetWindowScaleDPI();
    info.dpiScale = std::max(dpi.x, dpi.y);
    if (info.dpiScale < 1.0f) info.dpiScale = 1.0f;
#endif

#if defined(__ANDROID__)
    android_app* appState = GetAndroidApp();
    info.assets = std::make_unique<AndroidAssetProvider>(appState->activity->assetManager);
    auto envContent = info.assets->readFileText(config::ENV_FILE);
    if (envContent) {
        EnvLoader::loadFromContent(*envContent);
    }
    info.dbPath = std::string("/data/data/com.theword.app/app_storage/") + config::DB_FILE;
#elif defined(__EMSCRIPTEN__)
    info.assets = std::make_unique<FileAssetProvider>();
    info.dbPath = std::string("/persistent/") + config::DB_FILE;
#else
    info.assets = std::make_unique<FileAssetProvider>();
    EnvLoader::load(config::ENV_FILE);
    std::string home = getenv("HOME") ? getenv("HOME") : ".";
    info.dbPath = home + "/" + config::DB_DIR + "/" + config::DB_FILE;
#endif

    return info;
}

std::unique_ptr<IHttpClient> CreateHttpClient() {
#if defined(__EMSCRIPTEN__)
    return std::make_unique<EmscriptenClient>();
#elif defined(THEWORD_HAS_HTTP)
    return std::make_unique<CurlHttpClient>();
#else
    return nullptr;
#endif
}

bool ShouldQuit() {
#if defined(__ANDROID__)
    android_app* app = GetAndroidApp();
    return !app || !app->window;
#else
    return false;
#endif
}

bool OpenURL(const char* url) {
#if defined(__ANDROID__)
    android_app* app = GetAndroidApp();
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);

    jclass activityClass = env->GetObjectClass(app->activity->clazz);
    jmethodID startActivity = env->GetMethodID(
        activityClass, "startActivity", "(Landroid/content/Intent;)V");

    jclass intentClass = env->FindClass("android/content/Intent");
    jmethodID intentCtor = env->GetMethodID(intentClass, "<init>", "(Ljava/lang/String;)V");
    jstring actionView = env->NewStringUTF("android.intent.action.VIEW");
    jobject intent = env->NewObject(intentClass, intentCtor, actionView);

    jclass uriClass = env->FindClass("android/net/Uri");
    jmethodID parse = env->GetStaticMethodID(
        uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
    jstring uriStr = env->NewStringUTF(url);
    jobject uri = env->CallStaticObjectMethod(uriClass, parse, uriStr);

    jmethodID setData = env->GetMethodID(
        intentClass, "setData", "(Landroid/net/Uri;)Landroid/content/Intent;");
    env->CallObjectMethod(intent, setData, uri);

    env->CallVoidMethod(app->activity->clazz, startActivity, intent);

    env->DeleteLocalRef(uriStr);
    env->DeleteLocalRef(uri);
    env->DeleteLocalRef(intent);
    env->DeleteLocalRef(actionView);

    app->activity->vm->DetachCurrentThread();
    return true;

#elif defined(__EMSCRIPTEN__)
    std::string js = std::string("window.open('") + url + "', '_blank')";
    emscripten_run_script(js.c_str());
    return true;

#elif defined(_WIN32)
    // Windows: use ShellExecute
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", url);
    return system(cmd) == 0;

#else
    // Linux/macOS: use xdg-open / open
    std::string cmd = std::string("xdg-open \"") + url + "\" 2>/dev/null || open \"" + url + "\" 2>/dev/null";
    return system(cmd.c_str()) == 0;
#endif
}

void WriteLog(LogLevel level, const char* message) {
#if defined(__ANDROID__)
    int androidPriority;
    switch (level) {
        case LogLevel::DEBUG: androidPriority = ANDROID_LOG_DEBUG; break;
        case LogLevel::INFO:  androidPriority = ANDROID_LOG_INFO;  break;
        case LogLevel::WARN:  androidPriority = ANDROID_LOG_WARN;  break;
        case LogLevel::ERROR: androidPriority = ANDROID_LOG_ERROR; break;
        default:              androidPriority = ANDROID_LOG_INFO;  break;
    }
    __android_log_print(androidPriority, "TheWord", "%s", message);
#else
    FILE* out = stdout;
    if (level == LogLevel::WARN || level == LogLevel::ERROR) {
        out = stderr;
    }
    fprintf(out, "%s\n", message);
    fflush(out);
#endif
}

std::string GetClipboard() {
#if defined(__ANDROID__)
    android_app* app = GetAndroidApp();
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);

    jclass ctxClass = env->GetObjectClass(app->activity->clazz);
    jmethodID getSystemService = env->GetMethodID(
        ctxClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring clipService = env->NewStringUTF("clipboard");
    jobject clipboardMgr = env->CallObjectMethod(
        app->activity->clazz, getSystemService, clipService);

    jclass cmClass = env->FindClass("android/content/ClipboardManager");
    jmethodID getText = env->GetMethodID(
        cmClass, "getText", "()Ljava/lang/CharSequence;");
    jobject charSeq = env->CallObjectMethod(clipboardMgr, getText);

    std::string result;
    if (charSeq) {
        jclass charSeqClass = env->GetObjectClass(charSeq);
        jmethodID toString = env->GetMethodID(
            charSeqClass, "toString", "()Ljava/lang/String;");
        jstring jstr = (jstring)env->CallObjectMethod(charSeq, toString);
        const char* utf = env->GetStringUTFChars(jstr, nullptr);
        if (utf) {
            result = utf;
            env->ReleaseStringUTFChars(jstr, utf);
        }
        env->DeleteLocalRef(jstr);
    }

    env->DeleteLocalRef(clipService);
    env->DeleteLocalRef(clipboardMgr);
    if (charSeq) env->DeleteLocalRef(charSeq);

    app->activity->vm->DetachCurrentThread();
    return result;

#else
    const char* text = GetClipboardText();
    return text ? std::string(text) : std::string();
#endif
}

void SetClipboard(const std::string& text) {
#if defined(__ANDROID__)
    android_app* app = GetAndroidApp();
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);

    jclass ctxClass = env->GetObjectClass(app->activity->clazz);
    jmethodID getSystemService = env->GetMethodID(
        ctxClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring clipService = env->NewStringUTF("clipboard");
    jobject clipboardMgr = env->CallObjectMethod(
        app->activity->clazz, getSystemService, clipService);

    jclass cmClass = env->FindClass("android/content/ClipboardManager");
    jmethodID setPrimaryClip = env->GetMethodID(
        cmClass, "setPrimaryClip", "(Landroid/content/ClipData;)V");

    jclass clipDataClass = env->FindClass("android/content/ClipData");
    jmethodID newPlainText = env->GetStaticMethodID(
        clipDataClass, "newPlainText",
        "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Landroid/content/ClipData;");

    jstring label = env->NewStringUTF("TheWord");
    jstring content = env->NewStringUTF(text.c_str());
    jobject clip = env->CallStaticObjectMethod(
        clipDataClass, newPlainText, label, content);

    env->CallVoidMethod(clipboardMgr, setPrimaryClip, clip);

    env->DeleteLocalRef(label);
    env->DeleteLocalRef(content);
    env->DeleteLocalRef(clip);
    env->DeleteLocalRef(clipService);
    env->DeleteLocalRef(clipboardMgr);

    app->activity->vm->DetachCurrentThread();
#else
    SetClipboardText(text.c_str());
#endif
}

void ShowKeyboard() {
#if defined(__ANDROID__)
    android_app* app = GetAndroidApp();
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);

    jclass activityClass = env->GetObjectClass(app->activity->clazz);
    jmethodID method = env->GetMethodID(activityClass, "showKeyboard", "()V");
    if (method) {
        env->CallVoidMethod(app->activity->clazz, method);
    }
    env->DeleteLocalRef(activityClass);

    app->activity->vm->DetachCurrentThread();
#endif
}

void HideKeyboard() {
#if defined(__ANDROID__)
    android_app* app = GetAndroidApp();
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);

    jclass activityClass = env->GetObjectClass(app->activity->clazz);
    jmethodID method = env->GetMethodID(activityClass, "hideKeyboard", "()V");
    if (method) {
        env->CallVoidMethod(app->activity->clazz, method);
    }
    env->DeleteLocalRef(activityClass);

    app->activity->vm->DetachCurrentThread();
#endif
}

bool HasTouchInput() {
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    return true;
#else
    return false;
#endif
}

} } // namespace theword::core::platform
