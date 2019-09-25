#include <jni.h>

#include <string>
#include <thread>
#include <array>
#include <set>

#include <android/native_window_jni.h>
#include <android/log.h>

#include "librealsense2/rs.hpp"

// trim from end (in place)
static inline void rtrim(std::string &s, char c) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [c](int ch) {
        return (ch != c);
    }).base(), s.end());
}

inline std::string api_version_to_string(int version) {
    if (version / 10000 == 0) {
        std::stringstream ss;
        ss << version;
        return ss.str();
    }
    std::stringstream ss;
    ss << (version / 10000) << "." << (version % 10000) / 100 << "." << (version % 100);
    return ss.str();
}

constexpr int clamp(int x) {
    int32_t t = x;
    return ((t) > 255 ? 255 : t < 0 ? 0 : t);
}

static int device_error_count = 0;
class DevicePipe {
public:
    DevicePipe(rs2::context& ctx, int device) : pipe(ctx), enabled(false), isWorking(false) {
        for(auto it = surface.begin(); it != surface.end(); ++it) {
            *it = nullptr;
        }

        auto devs = ctx.query_devices();
        if (device < 0 || device >= devs.size()) return;
        sn = devs[device].get_info(rs2_camera_info::RS2_CAMERA_INFO_SERIAL_NUMBER);
        cfg.enable_device(sn);
    }

    void Start() {
        __android_log_print(ANDROID_LOG_INFO, "DBG", "pipe.start %s BEGIN %d", sn.c_str(), device_error_count);
        try {
            auto stream_profiles = pipe.start(cfg);
            isWorking = true;

#if 0
            __android_log_print(ANDROID_LOG_INFO, "DBG", "pipe.wait_for_frames BEGIN");
            pipe.wait_for_frames();
            auto dev = stream_profiles.get_device();
            auto sensors = dev.query_sensors();
            for (auto sensor: sensors) {
                if (sensor.is<rs2::depth_sensor>()) {
                    __android_log_print(ANDROID_LOG_INFO, "DBG", "RS2_OPTION_VISUAL_PRESET BEGIN");
                    sensor.set_option(rs2_option::RS2_OPTION_VISUAL_PRESET, rs2_rs400_visual_preset::RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY);
                    break;
                }
            }
#endif
        } catch (std::exception& e) {
            isWorking = false;
            device_error_count ++;
            __android_log_print(ANDROID_LOG_ERROR, "DBG", "===== > pipe.start %s ERROR: %s", sn.c_str(), e.what());
        }
        __android_log_print(ANDROID_LOG_INFO, "DBG", "pipe.start %s END", sn.c_str());
    }
    void Stop() {
        __android_log_print(ANDROID_LOG_INFO, "DBG", "pipe.stop %s", sn.c_str());
        if (isWorking) {
            pipe.stop();
            isWorking = false;
        }
        cfg.disable_all_streams();
        enabled = false;
    }

    void EnableStream(int stream, int width, int height, int fps, int format, ANativeWindow * surface) {
        cfg.enable_stream(static_cast<rs2_stream>(stream), width, height, static_cast<rs2_format>(format), fps);

        this->surface[stream - 1] = surface;
        this->enabled = true;
    }

    bool GrabFrames(rs2::frameset& frames) {
        if (!isWorking) return false;
#if 1
        return pipe.poll_for_frames(&frames);
#else
        frames = pipe.wait_for_frames();
        return true;
#endif
    }
    ANativeWindow* GetSurface(rs2_stream stream) {
        if (stream <= rs2_stream::RS2_STREAM_ANY || stream > rs2_stream::RS2_STREAM_INFRARED) {
            return nullptr;
        }
        return surface[stream - 1];
    }
    void ReleaseSurface() {
        for (int i = 0; i < surface.size(); ++i) {
            if (surface[i]) {
                ANativeWindow_release(surface[i]);
                surface[i] = nullptr;
            }
        }
    }
    bool IsEnabled(void) {
        return enabled;
    }

private:
    rs2::pipeline pipe;
    rs2::config cfg;
    std::string sn;
    std::array<ANativeWindow*, 3> surface;
    bool enabled;
    bool isWorking;
};

class RealSenseWrapper {
public:
    RealSenseWrapper() {
        //rs2::log_to_file(rs2_log_severity::RS2_LOG_SEVERITY_ERROR, "/sdcard/Download/realsense_app.log");
        rs2::log_to_console(rs2_log_severity::RS2_LOG_SEVERITY_ERROR);

        ctx = std::make_shared<rs2::context>();
        std::stringstream ss;
        {
            rs2_error* e = nullptr;
            ss << "librealsense2 VERSION: " << api_version_to_string(rs2_get_api_version(&e)) << std::endl;
        }
        auto devices = ctx->query_devices();
        ss << "Devices count: " << devices.size() << std::endl;
        for (uint32_t i = 0U; i < devices.size(); ++i) {
            auto device = devices[i];
            auto name = device.get_info(rs2_camera_info::RS2_CAMERA_INFO_NAME);
            auto sn = device.get_info(rs2_camera_info::RS2_CAMERA_INFO_SERIAL_NUMBER);
            auto fw = device.get_info(rs2_camera_info::RS2_CAMERA_INFO_FIRMWARE_VERSION);
            ss << " DEVICE: " << i << " NAME: " << name << " SN: "<< sn << " FW: " << fw << std::endl;
        }
        ss << std::endl;

        for (const auto& sensor : ctx->query_all_sensors()) {

            auto name = sensor.get_info(rs2_camera_info::RS2_CAMERA_INFO_NAME);
            auto sn = sensor.get_info(rs2_camera_info::RS2_CAMERA_INFO_SERIAL_NUMBER);

            __android_log_print(ANDROID_LOG_INFO, "DEVICE", "NAME: %s SN: %s", name, sn);
            ss << "SENSOR NAME: " << name << " SN: " << sn << std::endl;
            for(const auto& profile : sensor.get_stream_profiles()) {
                auto vprofile = profile.as<rs2::video_stream_profile>();
                __android_log_print(ANDROID_LOG_INFO, "PROFILE", "PROFILE %d = %s, %s, %d, %d, %d",
                                    profile.unique_id(),
                                    vprofile.stream_name().c_str(),
                                    rs2_format_to_string(vprofile.format()),
                                    vprofile.width(),
                                    vprofile.height(),
                                    vprofile.fps());
                ss << "PROFILE " << profile.unique_id() << " = " << vprofile.stream_name() << ", " << rs2_format_to_string(vprofile.format()) << ", " << vprofile.width() << ", " << vprofile.height() << ", " << vprofile.fps() << std::endl;
            }
        }

        profileList = ss.str();

        devices_info.resize(devices.size());
        for (auto it = devices_info.begin(); it != devices_info.end(); ++it) {
            *it = nullptr;
        }
    }

    ~RealSenseWrapper() {
        for (uint i = 0; i < devices_info.size(); ++i) {
            auto dev = devices_info.at(i);
            if (dev) {
                delete dev;
            }
        }
        ctx.reset();
    }

    void EnableStream(int device, int stream, int width, int height, int fps, int format, ANativeWindow * surface) {
        if (device < 0 || device >= devices_info.size()) return;

        if (!devices_info[device]) devices_info[device] = new DevicePipe(*ctx, device);

        devices_info[device]->EnableStream(stream, width, height, fps, format, surface);

        ANativeWindow_setBuffersGeometry(surface, width, height, WINDOW_FORMAT_RGBX_8888);
    }

    int Play() {
        isRunning = true;

        worker = std::thread(&RealSenseWrapper::FrameLoop, this);

        return 1;
    }

    void Stop() {
        isRunning = false;
        if (worker.joinable()) worker.join();

        for(auto dev: devices_info) {
            if (dev && dev->IsEnabled()) {
                dev->Stop();
            }
        }
    }

    void FrameLoop() {
#if 0
        __android_log_print(ANDROID_LOG_DEBUG, "RS native", "FrameLoop hardware_reset");
        {
            for (auto dev: ctx->query_devices()) {
                dev.hardware_reset();
            }

            std::this_thread::sleep_for(std::chrono::seconds(3));
            rs2::device_hub hub(*ctx);
            hub.wait_for_device();
        }
#endif
        __android_log_print(ANDROID_LOG_DEBUG, "RS native", "FrameLoop BEGIN");

        for(auto dev: devices_info) {
            if (dev && dev->IsEnabled()) {
                dev->Start();
            }
        }

        rs2::colorizer colorizer;
        rs2::frameset frames;
        while (isRunning) {
            for (auto dev: devices_info) {
                if (!dev) continue;
                if (!dev->IsEnabled()) continue; // Turned Off

                if (dev->GrabFrames(frames)) {
                    {
                        auto depth = frames.get_depth_frame();
                        if (depth) {
                            auto colorize_depth = colorizer.process(depth);
                            Draw(dev->GetSurface(rs2_stream::RS2_STREAM_DEPTH), colorize_depth);
                        }
                    }
                    {
                        auto color = frames.get_color_frame();
                        if (color) {
                            Draw(dev->GetSurface(rs2_stream::RS2_STREAM_COLOR), color);
                        }
                    }
                    {
                        auto ir = frames.get_infrared_frame();
                        if (ir) {
                            Draw(dev->GetSurface(rs2_stream::RS2_STREAM_INFRARED), ir);
                        }
                    }
                }
            }
        }

        for (auto dev: devices_info) {
            if (dev) {
                dev->ReleaseSurface();
            }
        }
        __android_log_print(ANDROID_LOG_DEBUG, "RS native", "FrameLoop END");
    }

    void Draw(ANativeWindow* window, rs2::frame& frame) {
        if (!window) return;

        auto vframe = frame.as<rs2::video_frame>();
        auto profile = vframe.get_profile().as<rs2::video_stream_profile>();
        ANativeWindow_Buffer buffer = {};
        ARect rect = {0, 0, profile.width(), profile.height()};

        if (ANativeWindow_lock(window, &buffer, &rect) == 0) {

            if (profile.format() == rs2_format::RS2_FORMAT_RGB8) {
                DrawRGB8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(),
                         profile.height(), vframe.get_stride_in_bytes());
            } else if (profile.format() == rs2_format::RS2_FORMAT_BGR8) {
                DrawBGR8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(),
                         profile.height(), vframe.get_stride_in_bytes());
            } else if (profile.format() == rs2_format::RS2_FORMAT_RGBA8) {
                DrawRGBA8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(),
                         profile.height(), vframe.get_stride_in_bytes());
            } else if (profile.format() == rs2_format::RS2_FORMAT_BGRA8) {
                DrawBGRA8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(),
                         profile.height(), vframe.get_stride_in_bytes());
            } else if (profile.format() == rs2_format::RS2_FORMAT_Y8) {
                DrawY8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
            } else if (profile.format() == rs2_format::RS2_FORMAT_Y16) {
                DrawY16(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
            } else if (profile.format() == rs2_format::RS2_FORMAT_YUYV) {
                DrawYUYV(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
            } else if (profile.format() == rs2_format::RS2_FORMAT_UYVY) {
                DrawUYVY(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
            }

            ANativeWindow_unlockAndPost(window);
        } else {
            __android_log_print(ANDROID_LOG_DEBUG, "RS NATIVE", "ANativeWindow_lock FAILED");
        }
    }

    void DrawRGB8(ANativeWindow_Buffer& buffer, const uint8_t * in, int in_width, int in_height, int in_stride_in_bytes) {
        uint8_t * out = static_cast<uint8_t *>(buffer.bits);
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x * 3 + 0];
                out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x * 3 + 1];
                out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x * 3 + 2];
                out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;
            }
        }
    }

    void DrawBGR8(ANativeWindow_Buffer& buffer, const uint8_t * in, int in_width, int in_height, int in_stride_in_bytes) {
        uint8_t * out = static_cast<uint8_t *>(buffer.bits);
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x * 3 + 2];
                out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x * 3 + 1];
                out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x * 3 + 0];
                out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;
            }
        }
    }

    void DrawRGBA8(ANativeWindow_Buffer& buffer, const uint8_t * in, int in_width, int in_height, int in_stride_in_bytes) {
        uint8_t * out = static_cast<uint8_t *>(buffer.bits);
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x * 4 + 0];
                out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x * 4 + 1];
                out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x * 4 + 2];
                out[y * buffer.stride * 4 + x * 4 + 3] = in[y * in_stride_in_bytes + x * 4 + 3];
            }
        }
    }

    void DrawBGRA8(ANativeWindow_Buffer& buffer, const uint8_t * in, int in_width, int in_height, int in_stride_in_bytes) {
        uint8_t * out = static_cast<uint8_t *>(buffer.bits);
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x * 4 + 2];
                out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x * 4 + 1];
                out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x * 4 + 0];
                out[y * buffer.stride * 4 + x * 4 + 3] = in[y * in_stride_in_bytes + x * 4 + 3];
            }
        }
    }

    void DrawY8(ANativeWindow_Buffer& buffer, const uint8_t * in, int in_width, int in_height, int in_stride_in_bytes) {
        uint8_t * out = static_cast<uint8_t *>(buffer.bits);
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x];
                out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x];
                out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x];
                out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;
            }
        }
    }
    void DrawY16(ANativeWindow_Buffer& buffer, const uint8_t * in, int in_width, int in_height, int in_stride_in_bytes) {
        uint8_t * out = static_cast<uint8_t *>(buffer.bits);
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x * 2 + 1];
                out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x * 2 + 1];
                out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x * 2 + 1];
                out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;
            }
        }
    }
    void DrawYUYV(ANativeWindow_Buffer& buffer, const uint8_t * in, int in_width, int in_height, int in_stride_in_bytes) {
        uint8_t * out = static_cast<uint8_t *>(buffer.bits);
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; x+=2) {
                int y1 = in[y * in_stride_in_bytes + x * 2 + 0];
                int u  = in[y * in_stride_in_bytes + x * 2 + 1];
                int y2 = in[y * in_stride_in_bytes + x * 2 + 2];
                int v  = in[y * in_stride_in_bytes + x * 2 + 3];

                int32_t c1 = y1 - 16;
                int32_t c2 = y2 - 16;
                int32_t d = u - 128;
                int32_t e = v - 128;

                int r1 = clamp((298 * c1 + 409 * e + 128) >> 8);
                int g1 = clamp((298 * c1 - 100 * d - 208 * e + 128) >> 8);
                int b1 = clamp((298 * c1 + 516 * d + 128) >> 8);
                int r2 = clamp((298 * c2 + 409 * e + 128) >> 8);
                int g2 = clamp((298 * c2 - 100 * d - 208 * e + 128) >> 8);
                int b2 = clamp((298 * c2 + 516 * d + 128) >> 8);

                out[y * buffer.stride * 4 + x * 4 + 0] = (uint8_t)r1;
                out[y * buffer.stride * 4 + x * 4 + 1] = (uint8_t)g1;
                out[y * buffer.stride * 4 + x * 4 + 2] = (uint8_t)b1;
                out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;

                out[y * buffer.stride * 4 + x * 4 + 4] = (uint8_t)r2;
                out[y * buffer.stride * 4 + x * 4 + 5] = (uint8_t)g2;
                out[y * buffer.stride * 4 + x * 4 + 6] = (uint8_t)b2;
                out[y * buffer.stride * 4 + x * 4 + 7] = 0xFF;
            }
        }
    }

    void DrawUYVY(ANativeWindow_Buffer& buffer, const uint8_t * in, int in_width, int in_height, int in_stride_in_bytes) {
        uint8_t * out = static_cast<uint8_t *>(buffer.bits);
        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; x+=2) {
                int y1 = in[y * in_stride_in_bytes + x * 2 + 1];
                int u  = in[y * in_stride_in_bytes + x * 2 + 0];
                int y2 = in[y * in_stride_in_bytes + x * 2 + 3];
                int v  = in[y * in_stride_in_bytes + x * 2 + 2];

                int32_t c1 = y1 - 16;
                int32_t c2 = y2 - 16;
                int32_t d = u - 128;
                int32_t e = v - 128;

                int r1 = clamp((298 * c1 + 409 * e + 128) >> 8);
                int g1 = clamp((298 * c1 - 100 * d - 208 * e + 128) >> 8);
                int b1 = clamp((298 * c1 + 516 * d + 128) >> 8);
                int r2 = clamp((298 * c2 + 409 * e + 128) >> 8);
                int g2 = clamp((298 * c2 - 100 * d - 208 * e + 128) >> 8);
                int b2 = clamp((298 * c2 + 516 * d + 128) >> 8);

                out[y * buffer.stride * 4 + x * 4 + 0] = (uint8_t)r1;
                out[y * buffer.stride * 4 + x * 4 + 1] = (uint8_t)g1;
                out[y * buffer.stride * 4 + x * 4 + 2] = (uint8_t)b1;
                out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;

                out[y * buffer.stride * 4 + x * 4 + 4] = (uint8_t)r2;
                out[y * buffer.stride * 4 + x * 4 + 5] = (uint8_t)g2;
                out[y * buffer.stride * 4 + x * 4 + 6] = (uint8_t)b2;
                out[y * buffer.stride * 4 + x * 4 + 7] = 0xFF;
            }
        }
    }

    void GetProfileFormatList(std::set<std::string>& out, int device, const std::string& stream, int width, int height, int fps) {
        //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "GetProfileList GetProfileFormatList BEGIN");
        auto devs = ctx->query_devices();
        if (device < 0 || device >= devs.size()) return;
        auto dev = devs[device];
        for (auto sensor : dev.query_sensors()) {
            for (auto profile : sensor.get_stream_profiles()) {
                auto vprofile = profile.as<rs2::video_stream_profile>();

                if (vprofile.width() == width && vprofile.height() == height && vprofile.fps() == fps && vprofile.stream_name().find(stream) == 0) {
                    std::string res = rs2_format_to_string(vprofile.format());

                    out.insert(res);
                }
            }
        }
        //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "GetProfileList GetProfileFormatList END");

    }
    void GetProfileFPSList(std::set<std::string>& out, int device, const std::string& stream, int width, int height) {
        //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "GetProfileList GetProfileFPSList BEGIN");
        auto devs = ctx->query_devices();
        if (device < 0 || device >= devs.size()) return;
        auto dev = devs[device];
        for (auto sensor : dev.query_sensors()) {
            for (auto profile : sensor.get_stream_profiles()) {
                auto vprofile = profile.as<rs2::video_stream_profile>();

                if (vprofile.width() == width && vprofile.height() == height && vprofile.stream_name().find(stream) == 0) {
                    std::stringstream ss;
                    ss << vprofile.fps();
                    std::string res = ss.str();

                    out.insert(res);
                }
            }
        }
        //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "GetProfileList GetProfileFPSList END");
    }
    void GetProfileResolutionList(std::set<std::string>& out, int device, const std::string& stream) {
        //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "GetProfileList GetProfileResolutionList BEGIN");
        auto devs = ctx->query_devices();
        if (device < 0 || device >= devs.size()) return;
        auto dev = devs[device];
        for (auto sensor : dev.query_sensors()) {
            for (auto profile : sensor.get_stream_profiles()) {
                auto vprofile = profile.as<rs2::video_stream_profile>();

                if (vprofile.stream_name().find(stream) == 0) {
                    std::stringstream ss;
                    ss << vprofile.width() << "x" << vprofile.height();
                    std::string res = ss.str();

                    out.insert(res);
                }
            }
        }
        //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "GetProfileList GetProfileResolutionList END");
    }
    void GetDevicesList(std::set<std::string>& out) {
        auto devs = ctx->query_devices();
        __android_log_print(ANDROID_LOG_DEBUG, "RS native", "GetDevicesList BEGIN");
        for (int i = 0; i < devs.size(); ++i) {
            std::stringstream ss;
            ss << i;
            out.insert(ss.str());
        }
        if (out.empty()) {
            out.insert("0");
        }
    }
    std::string& GetProfileList(int device, const std::string& stream, int width, int height, int fps) {
        //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "GetProfileList BEGIN");

        std::set<std::string> list;
        //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "GetProfileList list full %d", (int)list.size());
        if (fps != -1) {
            GetProfileFormatList(list, device, stream, width, height, fps);
        } else if (width != -1) {
            GetProfileFPSList(list, device, stream, width, height);
        } else if (stream.size() > 0) {
            GetProfileResolutionList(list, device, stream);
        } else if (device == -1) {
            GetDevicesList(list);
        }
        //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "GetProfileList list full %d", (int)list.size());

        std::stringstream ss;
        for (const auto i : list) {
            ss << i << ",";
        }
        log = ss.str();
        rtrim(log, ',');

        //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "GetProfileList END %s", log.c_str());
        return log;
    }

    std::string& GetLogMessage() {
        return profileList;
    }

private:
    std::shared_ptr<rs2::context> ctx;
    std::vector<DevicePipe*> devices_info;

    std::thread worker;
    bool isRunning;

    std::string log;
    std::string profileList;

public:
    static RealSenseWrapper * GetInstance() {
        return instance.get();
    }
    static void Init() {
        if (!instance) {
            instance = std::make_shared<RealSenseWrapper>();
        }
    }
    static void Cleanup() {
        if (instance) {
            instance.reset();
        }
    }
private:
    static std::shared_ptr<RealSenseWrapper> instance;
};

std::shared_ptr<RealSenseWrapper> RealSenseWrapper::instance;

JNIEXPORT jstring JNICALL
Java_com_example_realsense_1app_rs2_StreamToString(JNIEnv *env, jclass type, jint s) {

    const char * returnValue = "NULL";
    rs2_stream stream = static_cast<rs2_stream>(s);
    returnValue = rs2_stream_to_string(stream);
    return env->NewStringUTF(returnValue);
}

JNIEXPORT jstring JNICALL
Java_com_example_realsense_1app_rs2_FormatToString(JNIEnv *env, jclass type, jint f) {

    const char * returnValue = "NULL";
    rs2_format format = static_cast<rs2_format>(f);
    returnValue = rs2_format_to_string(format);
    return env->NewStringUTF(returnValue);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_realsense_1app_rs2_StreamFromString(JNIEnv *env, jclass type, jstring s_) {
    const char *s = env->GetStringUTFChars(s_, 0);

    int returnValue = -1;

    for (int i = rs2_stream::RS2_STREAM_ANY; i < rs2_stream::RS2_STREAM_COUNT; ++i) {
        if (strcmp(s, rs2_stream_to_string(static_cast<rs2_stream>(i))) == 0) {
            returnValue = i;
            break;
        }
    }

    env->ReleaseStringUTFChars(s_, s);

    return returnValue;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_realsense_1app_rs2_FormatFromString(JNIEnv *env, jclass type, jstring f_) {
    const char *f = env->GetStringUTFChars(f_, 0);

    int returnValue = -1;

    for (int i = rs2_format::RS2_FORMAT_ANY; i < rs2_format::RS2_FORMAT_COUNT; ++i) {
        if (strcmp(f, rs2_format_to_string(static_cast<rs2_format>(i))) == 0) {
            returnValue = i;
            break;
        }
    }

    env->ReleaseStringUTFChars(f_, f);

    return returnValue;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_init(JNIEnv *env, jclass type) {

    RealSenseWrapper::Init();

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_cleanup(JNIEnv *env, jclass type) {

    RealSenseWrapper::Cleanup();

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_enableStream(JNIEnv *env, jclass type,
                                                          jint device, jint stream,
                                                          jint width, jint height, jint fps,
                                                          jint format, jobject surface) {
    __android_log_print(ANDROID_LOG_DEBUG, "RS native", "MainActivity_enableStream %d %d, %d %d %d, %d", device, stream, width, height, fps, format);

    ANativeWindow* _surface = ANativeWindow_fromSurface(env, surface);
    //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "ANativeWindow_fromSurface %d", (int)(long)_surface);

    RealSenseWrapper::GetInstance()->EnableStream(
            device, stream, width, height, fps, format, _surface
    );
    //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "MainActivity_enableStream END");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_play(JNIEnv *env, jclass type) {

    __android_log_print(ANDROID_LOG_DEBUG, "RS native", "MainActivity_play BEGIN");
    RealSenseWrapper::GetInstance()->Play();
    __android_log_print(ANDROID_LOG_DEBUG, "RS native", "MainActivity_play END");

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_stop(JNIEnv *env, jclass type) {

    RealSenseWrapper::GetInstance()->Stop();

}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_realsense_1app_MainActivity_getProfileList(JNIEnv *env, jclass type,
                                                            jint device, jstring jstream, jint width, jint height, jint fps) {

    if (!RealSenseWrapper::GetInstance()) return env->NewStringUTF("");;

    const char *stream = env->GetStringUTFChars(jstream, nullptr);
    //__android_log_print(ANDROID_LOG_DEBUG, "RS native", "MainActivity_getProfileList %s, W:%d H:%d FPS:%d", stream, width, height, fps);

    std::string& list = RealSenseWrapper::GetInstance()->GetProfileList(device, stream, width, height, fps);

    env->ReleaseStringUTFChars(jstream, stream);

    return env->NewStringUTF(list.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_realsense_1app_MainActivity_logMessage(JNIEnv *env, jclass type) {

    std::string& log = RealSenseWrapper::GetInstance()->GetLogMessage();
    return env->NewStringUTF(log.c_str());
}