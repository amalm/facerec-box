#include <gst/rtsp-server/rtsp-server.h>

#include "skidata-gst-rtsp.h"
#include "skidata-gst-defines.h"

#include <stdio.h>
#include <stdlib.h>
#include "deepstream_config.h"
//#include "deepstream_sinks.h"
//#include "deepstream_common.h"

#include "deepstream_common.h"

#include <stdio.h>
#include <stdlib.h>


GstElement* createRtspSink()
{
    return gst_element_factory_make("fakesink", "fake-sink");
}

gboolean
start_rtsp_streaming (guint rtsp_port_num, guint updsink_port_num,
                      NvDsEncoderType enctype)
{
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory;
  char udpsrc_pipeline[512];

  char port_num_Str[64] = { 0 };
  char *encoder_name;

  if (enctype == NV_DS_ENCODER_H264) {
    encoder_name = (char*)"H264";
  } else if (enctype == NV_DS_ENCODER_H265) {
    encoder_name = (char*)"H265";
  } else {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
    return FALSE;
  }

  sprintf (udpsrc_pipeline,
      "( udpsrc name=pay0 port=%d caps=\"application/x-rtp, media=video, "
      "clock-rate=90000, encoding-name=%s, payload=96 \" )",
      updsink_port_num, encoder_name);

  sprintf (port_num_Str, "%d", rtsp_port_num);

  server = gst_rtsp_server_new ();

  //gst_rtsp_server_set_address(server, "10.16.70.122");
  g_object_set (server, "service", port_num_Str, NULL);

  mounts = gst_rtsp_server_get_mount_points (server);

  factory = gst_rtsp_media_factory_new ();
  gst_rtsp_media_factory_set_launch (factory, udpsrc_pipeline);

  gst_rtsp_mount_points_add_factory (mounts, "/skidata-car-perception", factory);

  g_object_unref (mounts);

  gst_rtsp_server_attach (server, NULL);

  g_print
      ("\n *** DeepStream: Launched RTSP Streaming at rtsp://localhost:%d/skidata-car-perception ***\n\n",
      rtsp_port_num);

  g_print("\n %s \n", gst_rtsp_server_get_service(server));
  g_print("\n %s \n", gst_rtsp_server_get_address(server));


  return TRUE;
}




gboolean
create_udpsink_bin (NvDsSinkEncoderConfig* config, NvDsSinkBinSubBin* bin)
{
  GstCaps *caps = NULL;
  gboolean ret = FALSE;
  gchar elem_name[50];
  gchar encode_name[50];
  gchar rtppay_name[50];

  //guint rtsp_port_num = g_rtsp_port_num++;
  gint uid = 0;

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin%d", uid);
  bin->bin = gst_bin_new (elem_name);
  if (!bin->bin) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_queue%d", uid);
  bin->queue = gst_element_factory_make (NVDS_ELEM_QUEUE, elem_name);
  if (!bin->queue) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_transform%d", uid);
  bin->transform = gst_element_factory_make (NVDS_ELEM_VIDEO_CONV, elem_name);
  if (!bin->transform) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_cap_filter%d", uid);
  bin->cap_filter = gst_element_factory_make (NVDS_ELEM_CAPS_FILTER, elem_name);
  if (!bin->cap_filter) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  caps = gst_caps_from_string ("video/x-raw(memory:NVMM), format=I420");
  g_object_set (G_OBJECT (bin->cap_filter), "caps", caps, NULL);

  g_snprintf (encode_name, sizeof (encode_name), "sink_sub_bin_encoder%d", uid);
  g_snprintf (rtppay_name, sizeof (rtppay_name), "sink_sub_bin_rtppay%d", uid);

  switch (config->codec) {
    case NV_DS_ENCODER_H264:
      //bin->codecparse = gst_element_factory_make ("h264parse", "h264-parser");
      bin->encoder = gst_element_factory_make (NVDS_ELEM_ENC_H264, encode_name);
      bin->rtppay = gst_element_factory_make ("rtph264pay", rtppay_name);
      break;
    case NV_DS_ENCODER_H265:
      //bin->codecparse = gst_element_factory_make ("h265parse", "h265-parser");
      bin->encoder = gst_element_factory_make (NVDS_ELEM_ENC_H265, encode_name);
      bin->rtppay = gst_element_factory_make ("rtph265pay", rtppay_name);
      break;
    default:
      goto done;
  }

  if (!bin->encoder) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", encode_name);
    goto done;
  }

  if (!bin->rtppay) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", rtppay_name);
    goto done;
  }

  g_object_set (G_OBJECT (bin->encoder), "bitrate", config->bitrate, NULL);
  g_object_set (G_OBJECT (bin->encoder), "iframeinterval", config->iframeinterval, NULL);

#ifdef IS_TEGRA
  g_object_set (G_OBJECT (bin->encoder), "preset-level", 1, NULL);
  g_object_set (G_OBJECT (bin->encoder), "insert-sps-pps", 1, NULL);
  g_object_set (G_OBJECT (bin->encoder), "bufapi-version", 1, NULL);
#else
  g_object_set (G_OBJECT (bin->transform), "gpu-id", config->gpu_id, NULL);
#endif

  g_snprintf (elem_name, sizeof (elem_name), "sink_sub_bin_udpsink%d", uid);
  bin->sink = gst_element_factory_make ("udpsink", elem_name);
  if (!bin->sink) {
    NVGSTDS_ERR_MSG_V ("Failed to create '%s'", elem_name);
    goto done;
  }

  g_object_set (G_OBJECT (bin->sink), "host", "224.224.255.255", "port",
      config->udp_port, "async", FALSE, "sync", 0, NULL);
      
  g_print("%d", config->udp_port);

  gst_bin_add_many (GST_BIN (bin->bin),
      bin->queue, bin->cap_filter, bin->transform,
      bin->encoder, //bin->codecparse, 
      bin->rtppay, bin->sink, NULL);

  NVGSTDS_LINK_ELEMENT (bin->queue, bin->transform);
  NVGSTDS_LINK_ELEMENT (bin->transform, bin->cap_filter);
  NVGSTDS_LINK_ELEMENT (bin->cap_filter, bin->encoder);
  NVGSTDS_LINK_ELEMENT (bin->encoder, bin->rtppay);
  NVGSTDS_LINK_ELEMENT (bin->rtppay, bin->sink);

  NVGSTDS_BIN_ADD_GHOST_PAD (bin->bin, bin->queue, "sink");

  ret = TRUE;

  ret = start_rtsp_streaming (config->rtsp_port, config->udp_port, config->codec);
  if (ret != TRUE) {
    g_print ("%s: start_rtsp_straming function failed\n", __func__);
  }

done:
  if (caps) {
    gst_caps_unref (caps);
  }
  if (!ret) {
    NVGSTDS_ERR_MSG_V ("%s failed", __func__);
  }
  return ret;
}

gboolean fillEncoderConfig (NvDsSinkEncoderConfig* config)
{
  //config->type = NV_DS_SINK_UDPSINK;
  config->codec = NV_DS_ENCODER_H265;
  config->bitrate = CONFIG_BITRATE;
  config->gpu_id = CONFIG_GPU_ID_RTSP;
  config->rtsp_port = CONFIG_RTSP_PORT;
  config->udp_port = CONFIG_UDP_PORT;
  config->iframeinterval = CONFIG_IFRAMEINTERVAL;

  return TRUE;
}

gboolean
addUdpAndStartRTSP(GstElement* pipeline, GstElement* lastElem)
{

  /* build config */
  gboolean ret = TRUE;

  NvDsSinkEncoderConfig config;

  fillEncoderConfig(&config);

  NvDsSinkBinSubBin bin;

  create_udpsink_bin(&config, &bin);

  /* now you have all the elements -> add them to pipeline */ 

  gst_bin_add(GST_BIN(pipeline), bin.bin);
  NVGSTDS_LINK_ELEMENT(lastElem, bin.bin);
  g_print("%d", config.codec);

  //start_rtsp_streaming (config.rtsp_port, config.udp_port, config.codec);
  return TRUE;
}
