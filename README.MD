# Face Recognition in a Box
Package a NVIDIA Jetson Nano with a camera in a box, use deep stream pipelines to detect, identify and decorate (rectangels and names of the person) and provide the result as RTSP stream to consume in for example VLC.

Structure:

* api: api definitions
* models: common models for detection, recognition, ...
* pipeline: deep stream pipelines
* enrollment: enrollment backend
* enrollment-ui: unerollment web ui
* hardware: design, specs, models, ... 