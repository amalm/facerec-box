#ifndef SKIDATA_GST_DEFINES_H
#define SKIDATA_GST_DEFINES_H

/* Tracker Config */
#define PGIE_CONFIG_FILE "skidata_primary_inference_config.txt"
#define SGIE_CONFIG_FILE "skidata_secondary_inference_config.txt"
#define TRACKER_CONFIG_FILE "skidata_tracker_config.txt"
#define MAX_TRACKING_ID_LEN 16
#define MAX_DISPLAY_LEN 64
#define PGIE_CLASS_ID_VEHICLE 0
#define PGIE_CLASS_ID_PERSON 2
#define MAX_TIME_STAMP_LEN 32
/* Message Broker Config */
#define MSCONV_CONFIG_FILE "skidata_kafka_config.txt"

/* Muxer batch formation timeout, for e.g. 40 millisec. Should ideally be set
 * based on the fastest source's framerate. */
#define MUXER_BATCH_TIMEOUT_USEC 4000000

#define TILED_OUTPUT_WIDTH 1280
#define TILED_OUTPUT_HEIGHT 720

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

#define CONFIG_GPU_ID_RTSP 0

/* Undistortion Config */
#define CONFIG_UNIQUE_ID 10
#define CONFIG_DISTORTION_COEFFICIENTS "0.1,0.1, -0.07, 0"
#define CONFIG_PROCESSING_WIDTH 1280
#define CONFIG_PROCESSING_HEIGHT 720

/* NVIDIA Decoder source pad memory feature. This feature signifies that source
 * pads having this capability will push GstBuffers containing cuda buffers. */
#define GST_CAPS_FEATURES_NVMM "memory:NVMM"

/* Encoder Config */
#define CONFIG_RTSP_PORT 8554
#define CONFIG_UDP_PORT 5400
#define CONFIG_BITRATE 350000
#define CONFIG_IFRAMEINTERVAL 10

/* Kafka Config */
#define CONFIG_KAFKA_FILE "skidata_kafka_config.txt"
#define CONFIG_CONN_STR "10.16.70.25;9092;skidata-car-perception"
#define KAFKA_PROTO_LIB "/opt/nvidia/deepstream/deepstream-4.0/lib/libnvds_kafka_proto.so" 

typedef enum
{
  SKIDATA_SINK_FAKE = 1,
  SKIDATA_SINK_EGL,
  SKIDATA_SINK_RTSP,
} SKIDATA_SINK_TYPE;

#define CONFIG_CURRENT_SINK 3

#define MESSAGE_INTERVAL 30

#endif