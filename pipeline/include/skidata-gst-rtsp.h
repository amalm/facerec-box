/*
 * Copyright (c) 2019, SKIDATA. All rights reserved.
 */
#ifndef SKIDATA_GST_H
#define SKIDATA_GST_H

#include <gst/gst.h>
#include <glib.h>

#include "deepstream_config.h"

#include "deepstream_sinks.h"



gboolean addUdpAndStartRTSP(GstElement* pipeline, GstElement* lastElem);

#endif