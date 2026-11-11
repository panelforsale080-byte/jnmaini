LOCAL_PATH := $(call my-dir)

# ============================================================================ #
include $(CLEAR_VARS)
LOCAL_MODULE            := libdobby
LOCAL_SRC_FILES         := dobby/libraries/$(TARGET_ARCH_ABI)/libdobby.a
include $(PREBUILT_STATIC_LIBRARY)


# ============================================================================ #
include $(CLEAR_VARS)
LOCAL_MODULE           := ZAGll

# Hide symbols (strip names from binary)
LOCAL_CFLAGS := -Wno-error=format-security -fvisibility=hidden -ffunction-sections -fdata-sections -w -fno-rtti -fno-exceptions -fpermissive
LOCAL_CFLAGS += -D_FORTIFY_SOURCE=2

LOCAL_CPPFLAGS := -Wno-error=format-security -fvisibility=hidden -ffunction-sections -fdata-sections -w -Werror -s -std=c++17
LOCAL_CPPFLAGS += -Wno-error=c++11-narrowing -fms-extensions -fno-rtti -fno-exceptions -fpermissive

# Strip all symbols (no debug info in final binary)
LOCAL_LDFLAGS += -Wl,--gc-sections,--strip-all,--exclude-libs,ALL -llog -s
LOCAL_LDFLAGS += -Wl,-z,relro,-z,now

LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_SRC_FILES :=  main.cpp \
                    codm_attach.cpp \
                    ANOGS/Fake_dlfcn.cpp \
                    ANOGS/Il2Cpp.cpp \
                    ANOGS/Tools.cpp \
                    ImGui/imgui.cpp \
                    ImGui/imgui_demo.cpp \
                    ImGui/imgui_draw.cpp \
                    ImGui/stb_image.cpp \
                    ImGui/imgui_tables.cpp \
                    ImGui/imgui_widgets.cpp \
                    ImGui/imgui_impl_android.cpp \
                    ImGui/imgui_impl_opengl3.cpp

LOCAL_LDLIBS := -landroid -lz -lEGL -lGLESv2 -lGLESv3
LOCAL_STATIC_LIBRARIES := libdobby

LOCAL_CPP_FEATURES := exceptions

include $(BUILD_SHARED_LIBRARY)
# ============================================================================ #