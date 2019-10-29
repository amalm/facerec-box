This directory contains models based on Caffe [1] implementation. MTCNN [2] is used for face detection. 
This bounding box is then extended by a factor 0.3 (except the extension outside image) to include the whole head, 
which is used as the input for networks (it's worth noting that this version is a bit tighter than the released one 
where the bounding box is extended by a factor 1.0).

Durining training, a region of 224x224 pixels is randomly cropped from each input, whose shorter size is resized to 256. 
The mean value of each channel is substracted for each pixel. The provided mean vector in the prototxt is in BGR order. 
You don't need to permute if you use opecv-based implementation. Bilinear interpolation is used for image resizing. 
More details can be found in the paper of VGGFace2.

Model:

resnet50_128_caffe: A 128-D dimensionality-reduction layer stacking at the final global-average pooling layer on ResNet-50 model.

References for implementation:

[1] Caffe: https://github.com/BVLC/caffe

[2] MTCNN: https://github.com/kpzhang93/MTCNN_face_detection_alignment

