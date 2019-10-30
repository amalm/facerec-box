/*
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "gstnvdsmeta.h"
#include "gstnvdsinfer.h"
#include <string.h>

//#include "skidata-gst-decode.h"
//#include "skidata-gst-defines.h"
//#include "skidata-gst-message-builder.h"

//#include "skidata-gst-tracker.h"
#include "skidata-gst-rtsp.h"


#define SINF_CONFIG_FILE "./configs/secondary_config.txt"

#define TRACKER_CONFIG_FILE "dstest2_tracker_config.txt"
#define MAX_TRACKING_ID_LEN 16
#define MAX_DISPLAY_LEN 64
#define MAX_ENROLLMENTS 20
#define NAME_SIZE 20
    

/* Define global variables:
 * `frame_number` & `pgie_classes_str` are used for writing meta to kitti;
 * `pgie_config`,`input_mp4`,`output_mp4`,`output_kitti` are configurable file paths parsed through command line. */
clock_t t_start;
clock_t t_end;

GstElement *pipeline, *video_full_processing_bin;
gint frame_number = 0;
gchar pgie_classes_str[4][32] = { "face", "license_plate", "make", "model" };

gchar *pgie_config = NULL;
gchar *input_mp4 = NULL;
gchar *output_mp4 = NULL;
gchar *output_kitti = NULL;

struct EnrollmentData {
  float vectorArray[128]; 
  char nameArray[NAME_SIZE];
};

/* initialize our gstreamer components for the app */

GMainLoop *loop = NULL;
GstElement *source = NULL, *decoder = NULL, *streammux = NULL,
    *queue_pgie = NULL, *pgie = NULL, *sgie1 = NULL, *nvtracker = NULL, *nvvidconv_osd =
    NULL, *osd = NULL, *queue_sink = NULL, *nvvidconv_sink =
    NULL, *filter_sink = NULL, *tiler = NULL, *videoconvert = NULL, *encoder = NULL, *muxer =
    NULL, *sink = NULL, *nvvidconv_src = NULL, *vidconv_src=NULL, *filter_src=NULL;
#ifdef PLATFORM_TEGRA
  GstElement *transform = NULL;
#endif
GstBus *bus = NULL;
guint bus_watch_id;
GstCaps *caps_filter_osd = NULL, *caps_filter_sink = NULL, *caps_filter_src = NULL;
gulong osd_probe_id = 0;
GstPad *osd_sink_pad = NULL, *tiler_sink_pad=NULL, *videopad = NULL;
NvDsDisplayMeta *display_meta = NULL;

/*
Read a file with all enrolled user vectors
*/
static struct EnrollmentData* read_enrolled_users() {
  FILE * fp;
  double points;
  size_t len = 0;
  ssize_t read;

  struct EnrollmentData *enrollmentArray = malloc(sizeof(struct EnrollmentData)*MAX_ENROLLMENTS);
  for (int i = 0; i < MAX_ENROLLMENTS; i++) {
    enrollmentArray[i].nameArray[0] = 0;
  }
  
  fp = fopen("/home/inno/enrollment/users.csv", "r");
  //rewind(fp);
  int i = 0;
  while(1){
    fscanf(fp, "%s", enrollmentArray[i].nameArray);
    int j = 0;
    while(fscanf(fp, "%lf", &points) == 1) {
      enrollmentArray[i].vectorArray[j] = points;
      j++;
    }
    i++;
    if(feof(fp) || i > MAX_ENROLLMENTS) {
      break;
    }
  }

  fclose(fp);
  return enrollmentArray;
}
/* osd_sink_pad_buffer_probe function will extract metadata received on OSD sink pad
 * and then update params for drawing rectangle and write bbox to kitti file. */
static GstPadProbeReturn
osd_sink_pad_buffer_probe (GstPad * pad, GstPadProbeInfo * info,
    gpointer u_data)
{

  GstBuffer *buf = (GstBuffer *) info->data;
  NvDsObjectMeta *obj_meta = NULL;
  NvDsMetaList * l_frame = NULL;
  NvDsMetaList * l_obj = NULL;
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta (buf);
  FILE *bbox_params_dump_file = NULL;
  gchar bbox_file[1024] = { 0 };
  NvDsInferTensorMeta *meta = NULL;
  
  struct EnrollmentData* enrollmentData = read_enrolled_users();

  for (l_frame = batch_meta->frame_meta_list; l_frame != NULL;
      l_frame = l_frame->next) {

    NvDsFrameMeta *frame_meta = (NvDsFrameMeta *) (l_frame->data);
    display_meta = nvds_acquire_display_meta_from_pool(batch_meta);
    NvOSD_TextParams *txt_params  = &display_meta->text_params[0];
    display_meta->num_labels = 1;
    txt_params->display_text = g_malloc0 (MAX_DISPLAY_LEN);

    /* Now set the offsets where the string should appear */
    txt_params->x_offset = 10;
    txt_params->y_offset = 12;

    if (frame_meta == NULL) {
      g_print ("NvDS Meta contained NULL meta \n");
      frame_number++;
      return GST_PAD_PROBE_OK;
    }

    if (output_kitti) {
      g_snprintf (bbox_file, sizeof (bbox_file) - 1, "%s/%06d.txt",
          output_kitti, frame_number);
        bbox_params_dump_file = fopen (bbox_file, "w");
    }

    for (l_obj = frame_meta->obj_meta_list; l_obj != NULL;
        l_obj = l_obj->next) {
      obj_meta = (NvDsObjectMeta *) (l_obj->data);

      /* Secondary inference: Iterate user metadata in object to search SGIE's tensor data */
      int faceFound = 0;
      char* nameFound = NULL;
      for (NvDsMetaList * l_user = obj_meta->obj_user_meta_list; l_user != NULL;
          l_user = l_user->next) {
        NvDsUserMeta *user_meta = (NvDsUserMeta *) l_user->data;
        if (user_meta->base_meta.meta_type != NVDSINFER_TENSOR_OUTPUT_META)
          continue;

        /* convert to tensor metadata */
        NvDsInferTensorMeta *meta =
            (NvDsInferTensorMeta *) user_meta->user_meta_data;
        for (unsigned int i = 0; i < meta->num_output_layers; i++) {
          NvDsInferLayerInfo *info = &meta->output_layers_info[i];
          info->buffer = meta->out_buf_ptrs_host[i];
        }

        NvDsInferDimsCHW dims;
        getDimsCHWFromDims (dims, meta->output_layers_info[0].dims);
        unsigned int numClasses = dims.c;
        float * outputCoverage = (float *) meta->output_layers_info[0].buffer;
        float thereshold = 1;
        
        for(int i = 0; i < MAX_ENROLLMENTS; i++) {
          if (enrollmentData[i].nameArray[0] == 0) {
            break;
          }
          float distance = 0;
          float percentage = 0;
          for (unsigned int c = 0; c < numClasses; c++) {
            distance = distance + pow((enrollmentData[i].vectorArray[c] - outputCoverage[c]), 2);
            //g_print(" %f ", outputCoverage[c]);
          }
          distance = sqrt (distance);
          //percentage = (thereshold * 100) / distance;
          if(distance < 220.0) {
            // NAME FOUND!
            faceFound = 1;
            nameFound = enrollmentData[i].nameArray;
            break;
          }
        }
      }
      // ########### Secondary inference END ############

      NvOSD_RectParams * rect_params = &(obj_meta->rect_params);
      NvOSD_TextParams * text_params = &(obj_meta->text_params);

      text_params->set_bg_clr = 1;
      text_params->font_params.font_size = 16;
      text_params->display_text = g_strdup("Unknown");
      if (nameFound) {
        text_params->display_text = g_strdup(nameFound);
      }

      /* Draw black patch to cover license plates (class_id = 1) */
      /* Stop showing Boxes by truning has_bg_color to 0.
         Do not change Alpha to 0.0, this throws some errors. */
      if (obj_meta->class_id == 1) {
        rect_params->border_width = 0;
        rect_params->has_bg_color = 0;
        rect_params->bg_color.red = 0.0;
        rect_params->bg_color.green = 0.0;
        rect_params->bg_color.blue = 0.0;
        rect_params->bg_color.alpha = 1.0;
      }
      
      /* Draw skin-color patch to cover faces (class_id = 0) */
      if (obj_meta->class_id == 0) {
        rect_params->border_width = 3;
        rect_params->has_bg_color = 0;
        if (faceFound) {
          rect_params->border_color.red = 0;
          rect_params->border_color.green = 1;
          rect_params->border_color.blue = 0;
        }
        else {
          rect_params->border_color.red = 1;
          rect_params->border_color.green = 0;
          rect_params->border_color.blue = 0;
        }
        rect_params->bg_color.alpha = 1.0;
        int left = (int) (rect_params->left);
        int top = (int) (rect_params->top);
        int right = left + (int) (rect_params->width);
        int bottom = top + (int) (rect_params->height);
        // g_print ("left: %d right: %d top: %d bottom: %d \n", left, right, top, bottom);
      }
      if (bbox_params_dump_file) {
        int left = (int) (rect_params->left);
        int top = (int) (rect_params->top);
        int right = left + (int) (rect_params->width);
        int bottom = top + (int) (rect_params->height);
        int class_index = obj_meta->class_id;
        char *text = pgie_classes_str[obj_meta->class_id];
        fprintf (bbox_params_dump_file,
            "%s 0.0 0 0.0 %d.00 %d.00 %d.00 %d.00 0.0 0.0 0.0 0.0 0.0 0.0 0.0\n",
            text, left, top, right, bottom);
      }

    }

    

    if (bbox_params_dump_file) {
      fclose (bbox_params_dump_file);
      bbox_params_dump_file = NULL;
    }
    // g_print("%s", &(obj_meta->text_params));
    nvds_add_display_meta_to_frame(frame_meta, display_meta);
  }
  frame_number++;

  return GST_PAD_PROBE_OK;
}


/* This bus callback function detects the error and state change in the main pipe 
 * and then export messages, export pipeline images or terminate the pipeline accordingly. */
static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      t_end = clock(); 
      clock_t t = t_end - t_start;
      double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds 
      double fps = frame_number/time_taken;
      g_print("\nThe program took %.2f seconds to redact %d frames, pref = %.2f fps \n\n", time_taken,frame_number,fps); 
      
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *error;
      gst_message_parse_error (msg, &error, &debug);
      g_printerr ("ERROR from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), error->message);
      g_free (debug);
      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_STATE_CHANGED:{
      GstState oldstate, newstate;
      gst_message_parse_state_changed (msg, &oldstate, &newstate, NULL);
      if (GST_ELEMENT (GST_MESSAGE_SRC (msg)) == pipeline) {
        switch (newstate) {
          case GST_STATE_PLAYING:
            g_print ("Pipeline running\n");
            GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
                GST_DEBUG_GRAPH_SHOW_ALL, "ds-app-playing");
            t_start = clock(); 
            break;
          case GST_STATE_PAUSED:
            if (oldstate == GST_STATE_PLAYING) {
              g_print ("Pipeline paused\n");
            }
            break;
          case GST_STATE_READY:
            // GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline), GST_DEBUG_GRAPH_SHOW_ALL,"ds-app-ready");
            if (oldstate == GST_STATE_NULL) {
              g_print ("Pipeline ready\n");
            } else {
              g_print ("Pipeline stopped\n");
            }
            break;
          default:
            break;
        }
      }
      break;
    }
    default:
      break;
  }
  return TRUE;
}

static gchar *
get_absolute_file_path (gchar *cfg_file_path, gchar *file_path)
{
  gchar abs_cfg_path[PATH_MAX + 1];
  gchar *abs_file_path;
  gchar *delim;

  if (file_path && file_path[0] == '/') {
    return file_path;
  }

  if (!realpath (cfg_file_path, abs_cfg_path)) {
    g_free (file_path);
    return NULL;
  }

  // Return absolute path of config file if file_path is NULL.
  if (!file_path) {
    abs_file_path = g_strdup (abs_cfg_path);
    return abs_file_path;
  }

  delim = g_strrstr (abs_cfg_path, "/");
  *(delim + 1) = '\0';

  abs_file_path = g_strconcat (abs_cfg_path, file_path, NULL);
  g_free (file_path);

  return abs_file_path;
}

#define CHECK_ERROR(error) \
    if (error) { \
        g_printerr ("Error while parsing config file: %s\n", error->message); \
        goto done; \
    }

#define CONFIG_GROUP_TRACKER "tracker"
#define CONFIG_GROUP_TRACKER_WIDTH "tracker-width"
#define CONFIG_GROUP_TRACKER_HEIGHT "tracker-height"
#define CONFIG_GROUP_TRACKER_LL_CONFIG_FILE "ll-config-file"
#define CONFIG_GROUP_TRACKER_LL_LIB_FILE "ll-lib-file"
#define CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS "enable-batch-process"
#define CONFIG_GPU_ID "gpu-id"


static gboolean
set_tracker_properties (GstElement *nvtracker)
{
  gboolean ret = FALSE;
  GError *error = NULL;
  gchar **keys = NULL;
  gchar **key = NULL;
  GKeyFile *key_file = g_key_file_new ();

  if (!g_key_file_load_from_file (key_file, TRACKER_CONFIG_FILE, G_KEY_FILE_NONE,
          &error)) {
    g_printerr ("Failed to load config file: %s\n", error->message);
    return FALSE;
  }

  keys = g_key_file_get_keys (key_file, CONFIG_GROUP_TRACKER, NULL, &error);
  CHECK_ERROR (error);

  for (key = keys; *key; key++) {
    if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_WIDTH)) {
      gint width =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_WIDTH, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "tracker-width", width, NULL);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_HEIGHT)) {
      gint height =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_HEIGHT, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "tracker-height", height, NULL);
    } else if (!g_strcmp0 (*key, CONFIG_GPU_ID)) {
      guint gpu_id =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GPU_ID, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "gpu_id", gpu_id, NULL);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_LL_CONFIG_FILE)) {
      char* ll_config_file = get_absolute_file_path (TRACKER_CONFIG_FILE,
                g_key_file_get_string (key_file,
                    CONFIG_GROUP_TRACKER,
                    CONFIG_GROUP_TRACKER_LL_CONFIG_FILE, &error));
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "ll-config-file", ll_config_file, NULL);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_LL_LIB_FILE)) {
      char* ll_lib_file = get_absolute_file_path (TRACKER_CONFIG_FILE,
                g_key_file_get_string (key_file,
                    CONFIG_GROUP_TRACKER,
                    CONFIG_GROUP_TRACKER_LL_LIB_FILE, &error));
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "ll-lib-file", ll_lib_file, NULL);
    } else if (!g_strcmp0 (*key, CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS)) {
      gboolean enable_batch_process =
          g_key_file_get_integer (key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS, &error);
      CHECK_ERROR (error);
      g_object_set (G_OBJECT (nvtracker), "enable_batch_process",
                    enable_batch_process, NULL);
    } else {
      g_printerr ("Unknown key '%s' for group [%s]", *key,
          CONFIG_GROUP_TRACKER);
    }
  }

  ret = TRUE;
done:
  if (error) {
    g_error_free (error);
  }
  if (keys) {
    g_strfreev (keys);
  }
  if (!ret) {
    g_printerr ("%s failed", __func__);
  }
  return ret;
}

/* In the main function, we create elements, configure the elements, assemble to elements to pipe and run the pipe */
int
main (int argc, char *argv[])
{
  /* initialize the app using GOption */
  GOptionContext *ctx;
  GError *err = NULL;
  GOptionEntry entries[] = {
    {"pgie_config", 'c', 0, G_OPTION_ARG_STRING, &pgie_config,
          "(required) configuration file for the nvinfer detector (primary gie)",
        NULL},
    // {"input_mp4", 'i', 0, G_OPTION_ARG_STRING, &input_mp4,
    //       "(optional) path to input mp4 file. If this is unset then usb webcam will be used as input", NULL},
    // {"output_mp4", 'o', 0, G_OPTION_ARG_STRING, &output_mp4,
    //       "(optional) path to output mp4 file. If this is unset then on-screen display will be used",
    //     NULL},
    {"output_kitti", 'k', 0, G_OPTION_ARG_STRING, &output_kitti,
          "(optional) path to the folder for containing output kitti files. If this is unset or \
the path does not exist then app won't output kitti files",
        NULL},
    {NULL},
  };
  ctx =
      g_option_context_new
      ("\n\n  'Fast Video Redaction App using NVIDIA Deepstream'\n  \
contact: Shuo Wang (shuow@nvidia.com)");
  g_option_context_add_main_entries (ctx, entries, NULL);
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
    g_print ("Failed to initialize: %s\n", err->message);
    g_clear_error (&err);
    g_option_context_free (ctx);
    return 1;
  }
  g_option_context_free (ctx);

  /* Check input arguments */
  if (!pgie_config) {
    g_printerr ("missing pgie configuration file\n");
    g_printerr ("run: %s --help for usage instruction\n", argv[0]);
    return -1;
  }

  /* Create gstreamer loop */
  loop = g_main_loop_new (NULL, FALSE);

  /* Create gstreamer pipeline */
  pipeline = gst_pipeline_new ("ds-redaction-pipeline");

  /* Create components for the input source */
  // if (input_mp4) {
  //   source = gst_element_factory_make ("filesrc", "file-source");
  //   g_object_set (G_OBJECT (source), "location", input_mp4, NULL);
  //   decoder = gst_element_factory_make ("decodebin", "decoder");
  //   g_signal_connect (decoder, "pad-added", G_CALLBACK (cb_newpad), NULL);

  //   gst_bin_add_many (GST_BIN (pipeline), source, decoder, NULL);
  //   gst_element_link (source, decoder);
  // } else {
    source = gst_element_factory_make ("v4l2src", "camera-source");
    g_object_set (G_OBJECT (source), "device", "/dev/video0", NULL);
    vidconv_src = gst_element_factory_make ("videoconvert", "vidconv_src");
    nvvidconv_src = gst_element_factory_make ("nvvideoconvert", "nvvidconv_src");
    filter_src = gst_element_factory_make ("capsfilter", "filter_src");
    g_object_set (G_OBJECT (nvvidconv_src), "nvbuf-memory-type", 0, NULL);
    caps_filter_src =
        gst_caps_from_string ("video/x-raw(memory:NVMM), format=NV12, width=1280, height=720, framerate=30/1");
    g_object_set (G_OBJECT (filter_src), "caps", caps_filter_src, NULL);
    gst_caps_unref (caps_filter_src);

    gst_bin_add_many (GST_BIN (pipeline), source, vidconv_src, nvvidconv_src, filter_src, NULL);
    gst_element_link_many (source, vidconv_src, nvvidconv_src, filter_src, NULL);
  // }

  /* Create main processing bin */
  video_full_processing_bin = gst_bin_new ("video-process-bin");

  /* Create nvstreammux instance to form batches from one or more sources. */
  streammux = gst_element_factory_make ("nvstreammux", "stream-muxer");
  g_object_set (G_OBJECT (streammux), "width", 1280, "height",
      720, "batch-size", 1, "batched-push-timeout", 40000, NULL);
  g_object_set (G_OBJECT (streammux), "nvbuf-memory-type", 0, NULL);

  gchar pad_name_sink[16] = "sink_0";
  videopad = gst_element_get_request_pad (streammux, pad_name_sink);

  /* Use nvinfer to run inferencing on decoder's output,
   * behaviour of inferencing is set through config file.
   * Create components for the detection */
  queue_pgie = gst_element_factory_make ("queue", "queue_pgie");
  pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");
  g_object_set (G_OBJECT (pgie), "config-file-path", pgie_config, NULL);

  /* We need to have a tracker to track the identified objects */
  nvtracker = gst_element_factory_make ("nvtracker", "tracker");

  sgie1 = gst_element_factory_make("nvinfer", "secondary-nvinference-engine");
  g_object_set (G_OBJECT (sgie1), "config-file-path", SINF_CONFIG_FILE, NULL);

  tiler = gst_element_factory_make ("nvmultistreamtiler", "tiler");
  g_object_set (G_OBJECT (tiler), "rows", 1, "columns",
      (guint) ceil (1.0 *  1), "width", 1280, "height", 720,
      NULL);

  /* Use nvosd to render bbox/text on top of the input video.
   * Create components for the rendering */
  nvvidconv_osd = gst_element_factory_make ("nvvideoconvert", "nvvidconv_osd");
  osd = gst_element_factory_make ("nvdsosd", "nv-onscreendisplay");

  /* Create components for the output */

  // if (output_mp4) {
  //   /* If output file is set, then create components for encoding and exporting to mp4 file */
  //   queue_sink = gst_element_factory_make ("queue", "queue_sink");
  //   nvvidconv_sink = gst_element_factory_make ("nvvideoconvert", "nvvidconv_sink");
  //   filter_sink = gst_element_factory_make ("capsfilter", "filter_sink");
  //   caps_filter_sink = gst_caps_from_string ("video/x-raw, format=I420");
  //   g_object_set (G_OBJECT (filter_sink), "caps", caps_filter_sink, NULL);
  //   gst_caps_unref (caps_filter_sink);
  //   videoconvert = gst_element_factory_make ("videoconvert", "videoconverter");
  //   encoder = gst_element_factory_make ("avenc_mpeg4", "mp4-encoder");
  //   g_object_set (G_OBJECT (encoder), "bitrate", 1000000, NULL);
  //   muxer = gst_element_factory_make ("qtmux", "muxer");
  //   sink = gst_element_factory_make ("filesink", "nvvideo-renderer");
  //   g_object_set (G_OBJECT (sink), "location", output_mp4, NULL);

  // } else {
  	/* If output file is not set, use on-screen display as output */
#ifdef PLATFORM_TEGRA

    transform = gst_element_factory_make ("nvegltransform", "nvegl-transform");
    if(!transform) {
      g_printerr ("One tegra element could not be created. Exiting.\n");
      return -1;
    }
    sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");
    g_object_set (sink, "sync", FALSE, "max-lateness", -1,
      "async", FALSE, "qos", TRUE, NULL);

#else

    sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");
    g_object_set (sink, "sync", FALSE, "max-lateness", -1,
      "async", FALSE, "qos", TRUE, NULL);

    if (input_mp4) {
    	g_object_set (sink, "sync", TRUE, NULL);
    }

#endif
    		
  // }

  /* Check all components */
  if (!pipeline || !source || !video_full_processing_bin
      || !queue_pgie || !pgie || !nvtracker || !sgie1 || !nvvidconv_osd
      || !osd || !sink|| !tiler) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  if (!set_tracker_properties(nvtracker)) {
    g_printerr ("Failed to set tracker properties. Exiting.\n");
    return -1;
  }

  /* Set up the pipeline */
  gst_bin_add_many (GST_BIN (video_full_processing_bin), streammux,
      queue_pgie, nvtracker, pgie, sgie1, tiler, nvvidconv_osd, osd, NULL);
  /* link the elements together */
  gst_element_link_many (streammux, queue_pgie, nvtracker, pgie, sgie1, tiler, nvvidconv_osd, osd,
      NULL);

  gst_element_add_pad (video_full_processing_bin, gst_ghost_pad_new ("sink",
          videopad));
  gst_object_unref (videopad);
  gst_bin_add (GST_BIN (pipeline), video_full_processing_bin);

  /* link soure and video_full_processing_bin */
  if (!input_mp4){

    GstPad *sinkpad, *srcpad;

    sinkpad = gst_element_get_static_pad (video_full_processing_bin, "sink");
    if (!sinkpad) {
      g_printerr ("video_full_processing_bin request sink pad failed. Exiting.\n");
      return -1;
    }

    srcpad = gst_element_get_static_pad (filter_src, "src");
    if (!srcpad) {
      g_printerr ("filter_src request src pad failed. Exiting.\n");
      return -1;
    }

    if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
        g_printerr ("Failed to link pads. Exiting.\n");
        return -1;
    }

    gst_object_unref (sinkpad);
    gst_object_unref (srcpad);
  }

  // if (output_mp4) {
  //   gst_bin_add_many (GST_BIN (video_full_processing_bin),
  //       queue_sink, nvvidconv_sink, filter_sink, videoconvert, encoder, muxer,
  //       sink, NULL);

  //   /* link the elements together */
  //   gst_element_link_many (osd, queue_sink, nvvidconv_sink, filter_sink,
  //       videoconvert, encoder, muxer, sink, NULL);
  // } else {

  // --------------------------------------------------
  // COMMENT OUT FOR RTSP, COMMENT IN FOR DISPLAY
  // --------------------------------------------------
  // ---------------------START-----------------------------
#ifdef PLATFORM_TEGRA
    gst_bin_add_many (GST_BIN (video_full_processing_bin), transform, sink, NULL);
    gst_element_link_many (osd, transform , sink, NULL);
#else
    gst_bin_add_many (GST_BIN (video_full_processing_bin), sink, NULL);
    gst_element_link (osd, sink);
#endif  	
// ------------------------END--------------------------
  // }
  



  /* add probe to get informed of the meta data generated, we add probe to
   * the sink pad of the osd element, since by that time, the buffer would have
   * had got all the metadata. */
  osd_sink_pad = gst_element_get_static_pad (osd, "sink");
  if (!osd_sink_pad)
    g_print ("Unable to get sink pad\n");

  /* Add probe to get informed of the meta data generated, we add probe to
   * the sink pad of tiler element which is just after all SGIE elements.
   * Since by that time, GstBuffer would have had got all SGIEs tensor
   * metadata. */
  tiler_sink_pad = gst_element_get_static_pad (tiler, "sink");
  gst_pad_add_probe (tiler_sink_pad, GST_PAD_PROBE_TYPE_BUFFER, osd_sink_pad_buffer_probe, NULL, NULL);


  /* Set the pipeline to "playing" state */
  // if (input_mp4) {
  // 	g_print ("Now playing: %s\n", input_mp4);
  // } else {
  	g_print ("Now playing from webcam\n");
  // }
  
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  GstElement* lastElem = osd;

  // --------------------------------------------------
  // COMMENT OUT FOR DISPLAY, COMMENT IN FOR RTSP
  // --------------------------------------------------
  //addUdpAndStartRTSP(pipeline, lastElem);

  GST_DEBUG_BIN_TO_DOT_FILE((GstBin*) pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");

  /* Wait till pipeline encounters an error or EOS */
  g_main_loop_run (loop);
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
                GST_DEBUG_GRAPH_SHOW_ALL, "ds-app-playing");

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);
  return 0;

}
