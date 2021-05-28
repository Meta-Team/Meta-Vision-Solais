#include <cstdlib>
#include <gst/gst.h>
#include <gst/gstinfo.h>
#include <gst/app/gstappsink.h>
#include <glib-unix.h>

#include <iostream>
#include <sstream>
#include <thread>

using namespace std;

#define USE(x) ((void)(x))

static GstPipeline *gst_pipeline = nullptr;
static string launch_string;
static int sleep_count = 0;
static int eos = 0;
static int frame_count = 0;

static void appsink_eos(GstAppSink *appsink, gpointer user_data) {
    printf("app sink receive eos\n");
    eos = 1;
}

static GstFlowReturn new_buffer(GstAppSink *appsink, gpointer user_data) {
    GstSample *sample = NULL;

    g_signal_emit_by_name(appsink, "pull-sample", &sample, NULL);

    if (sample) {
        GstBuffer *buffer = NULL;
        GstCaps *caps = NULL;
        GstMapInfo map = {0};
        gint width, height;

        caps = gst_sample_get_caps(sample);
        if (!caps) {
            printf("could not get caps\n");
        }
        GstStructure *st = gst_caps_get_structure(caps, 0);
        buffer = gst_sample_get_buffer(sample);
        gst_buffer_map(buffer, &map, GST_MAP_READ);
        gst_structure_get_int(st, "width", (gint * ) & width);
        gst_structure_get_int(st, "height", (gint * ) & height);

//        printf("%dx%d, map.size = %lu\n", width, height, map.size);
        frame_count++;

        gst_buffer_unmap(buffer, &map);

        gst_sample_unref(sample);
    } else {
        g_print("could not make snapshot\n");
    }

    return GST_FLOW_OK;
}

int main(int argc, char **argv) {
    USE(argc);
    USE(argv);

    gst_init(&argc, &argv);

    GMainLoop *main_loop;
    main_loop = g_main_loop_new(NULL, FALSE);
    GstAppSinkCallbacks callbacks = {appsink_eos, NULL, new_buffer};

    string launch_string = "v4l2src device=/dev/video0 io-mode=2 ! "
                           "image/jpeg, width=(int)1280, height=(int)720, framerate=120/1 ! "
                           "nvjpegdec ! video/x-raw ! "
                           "nvvidconv ! video/x-raw, format=BGR !"
//                           "videoconvert ! video/x-raw, format=BGR ! "
                           "appsink name=mysink";

    g_print("Using launch string: %s\n", launch_string.c_str());

    GError *error = nullptr;
    gst_pipeline = (GstPipeline *) gst_parse_launch(launch_string.c_str(), &error);

    if (gst_pipeline == nullptr) {
        g_print("Failed to parse launch: %s\n", error->message);
        return -1;
    }
    if (error) g_error_free(error);

    GstElement *appsink_ = gst_bin_get_by_name(GST_BIN(gst_pipeline), "mysink");
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink_), &callbacks, NULL, NULL);

    gst_element_set_state((GstElement *) gst_pipeline, GST_STATE_PLAYING);

    printf("Check point 0\n");

    while (eos == 0) {
        sleep(1);
        sleep_count++;
        printf("FPS: %d frames/s\n", frame_count);
        frame_count = 0;
    }

    printf("Check point 1\n");

    gst_element_set_state((GstElement *) gst_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(gst_pipeline));
    g_main_loop_unref(main_loop);

    printf("Check point 2\n");

    return 0;
}
